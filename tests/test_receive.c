#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"
#include "../tdtimer.h"

/* Scripted USB reports and a virtual clock: no hardware or real sleeps. */
static struct {
    int delay, result;
    uint8_t payload[MAX_REPORT_LENGTH];
} replies[8];
static int reply_count, read_count, send_count, repeat, send_error;
static int timeouts[32], lengths[32], commands[32];
static uint64_t now;
static jmp_buf error_jump;
static int expecting_error;
static const char* error_message;

static void reset(void)
{
    memset(replies, 0, sizeof(replies));
    reply_count = read_count = send_count = repeat = send_error = 0;
    now = 10000;
}

static void reply(int delay, uint8_t type, uint8_t a, uint8_t b)
{
    assert(reply_count < 8);
    replies[reply_count].delay = delay;
    replies[reply_count].payload[0] = type;
    replies[reply_count].payload[1] = a;
    replies[reply_count].payload[2] = b;
    reply_count++;
}

uint64_t TdTimer_GetMilliseconds(void) { return now; }
void debug_print(const char* message, ...) { (void)message; }

void throw_exception(int exitcode, const char* message)
{
    assert(expecting_error);
    assert(exitcode == EXITCODE_DEVICE_IO_ERROR);
    error_message = message;
    longjmp(error_jump, 1);
}

#define EXPECT_ERROR(operation, message) do { \
    expecting_error = 1; \
    if (setjmp(error_jump) == 0) { operation; assert(!"Expected an error"); } \
    expecting_error = 0; \
    assert(strcmp(error_message, message) == 0); \
} while (0)

int TdHidSetReport(int* handle, unsigned char* buffer, int len, uint8_t type)
{
    (void)handle;
    assert(len > 1 && type == USB_HID_REPORT_TYPE_OUTPUT);
    assert(send_count < 32);
    commands[send_count++] = buffer[1];
    return send_error;
}

int TdHidGetReport(int* handle, unsigned char* buffer, int len, uint8_t type)
{
    (void)handle; (void)buffer; (void)len; (void)type;
    assert(!"Unexpected control report read");
    return TDHID_ERR_IO;
}

int TdHidReadInputReport(int* handle, unsigned char* buffer, int len, int timeout_ms)
{
    (void)handle;
    assert(read_count < 32 && len > 1 && timeout_ms >= 0);
    int index = repeat ? 0 : read_count;
    lengths[read_count] = len;
    timeouts[read_count++] = timeout_ms;
    if (index >= reply_count || (timeout_ms && replies[index].delay > timeout_ms)) {
        assert(timeout_ms > 0); /* A command must never wait indefinitely. */
        now += timeout_ms;
        return TDHID_ERR_TIMEOUT;
    }
    now += replies[index].delay;
    buffer[0] = 0;
    memcpy(buffer + 1, replies[index].payload, len - 1);
    return replies[index].result;
}

extern td_device_t* (*tdfa30604_import)(void);
extern td_device_t* (*tdfa30608_import)(void);
extern td_device_t* (*tdfa60220_import)(void);
extern td_device_t* (*tdfa60228_import)(void);
extern td_device_t* (*tdfa60250_import)(void);
extern td_device_t* (*tdpc0201_import)(void);
extern td_device_t* (*tdsn450_import)(void);
extern td_device_t* (*tdsn0700_import)(void);
extern td_device_t* (*tdsn5200_import)(void);
extern td_device_t* (*tdsn604xx_import)(void);
extern td_device_t* (*tdsn7200_import)(void);
extern td_device_t* (*tdsn7300_import)(void);
extern td_device_t* (*tdsn7360_import)(void);
extern td_device_t* (*tdsn7400_import)(void);

static void test_commands(td_context_t* context)
{
    reset();
    EXPECT_ERROR(tddev2_save_to_flash(context), ERROR_MSG_DEVICE_TIMEOUT);
    assert(send_count == 1 && commands[0] == OUTPACKET_SAVE);
    assert(now == 13000 && read_count == 1);

    reset();
    reply(1000, INPACKET_DUMP, 0, 0);
    repeat = 1;
    EXPECT_ERROR(tddev2_save_to_flash(context), ERROR_MSG_DEVICE_TIMEOUT);
    assert(now == 13000 && send_count == 1 && read_count == 3);
    assert(timeouts[0] == 3000 && timeouts[1] == 2000 && timeouts[2] == 1000);

    reset();
    reply(250, INPACKET_ACK, OUTPACKET_SET, 0);
    reply(250, INPACKET_DUMP, 0, 0);
    reply(250, INPACKET_ACK, OUTPACKET_SAVE, 0);
    assert(tddev2_save_to_flash(context) == 0);
    assert(read_count == 3 && send_count == 1);

    reset();
    reply(100, INPACKET_DUMP, 0, 0);
    reply(100, INPACKET_ACK, 0, 0); /* Legacy firmware omits the SET opcode. */
    assert(tddev2_write_devreg(context, 0x1234, 42) == 0);
    assert(read_count == 2 && send_count == 1);

    reset();
    EXPECT_ERROR(tddev2_write_devreg(context, 0x1234, 42), ERROR_MSG_DEVICE_TIMEOUT);
    assert(now == 13000 && send_count == 1); /* Never retry writes. */

    reset();
    reply(100, INPACKET_DEVREG, 0x35, 0x12);
    reply(100, INPACKET_DEVREG, 0x34, 0x12);
    replies[1].payload[3] = 42;
    assert(tddev2_read_devreg(context, 0x1234) == 42 && read_count == 2);

    reset();
    EXPECT_ERROR(tddev2_read_devreg(context, 0x1234), ERROR_MSG_DEVICE_TIMEOUT);
    assert(now == 19000 && send_count == 3 && read_count == 3);

    reset();
    reply(1000, INPACKET_DEVREG, 0x35, 0x12);
    repeat = 1;
    EXPECT_ERROR(tddev2_read_devreg(context, 0x1234), ERROR_MSG_DEVICE_TIMEOUT);
    assert(now == 19000 && send_count == 3 && read_count == 9);

    reset();
    reply(3000, 0, 0, 0);
    replies[0].result = TDHID_ERR_TIMEOUT;
    reply(100, INPACKET_DEVREG, 0x34, 0x12);
    replies[1].payload[3] = 43;
    assert(tddev2_read_devreg(context, 0x1234) == 43 && send_count == 2);

    reset();
    reply(0, 0, 0, 0);
    replies[0].result = TDHID_ERR_IO;
    EXPECT_ERROR(tddev2_read_devreg(context, 0), ERROR_MSG_DEVICE_IO_ERROR);
    assert(send_count == 1 && read_count == 1);

    reset();
    send_error = TDHID_ERR_IO;
    EXPECT_ERROR(tddev2_save_to_flash(context), ERROR_MSG_DEVICE_IO_ERROR);
    assert(read_count == 0);
}

static void test_devices(void)
{
    td_device_t* (*sensors[])(void) = { tdsn7200_import, tdsn7300_import };
    for (unsigned int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
        td_context_t context = { 0 };
        context.device_type = sensors[i]();
        reset();
        EXPECT_ERROR(context.device_type->get(&context), ERROR_MSG_DEVICE_TIMEOUT);
        assert(now == 13000 && send_count == 1);
        reset();
        reply(1000, INPACKET_DUMP, 0, 0);
        repeat = 1;
        EXPECT_ERROR(context.device_type->get(&context), ERROR_MSG_DEVICE_TIMEOUT);
        assert(now == 13000 && read_count == 3);
        reset();
        reply(100, INPACKET_DUMP, 0, 0);
        reply(100, 0x80, 0, 0);
        assert(context.device_type->get(&context) == 0 && read_count == 2);
        free(context.device_type);
    }

    td_context_t context = { 0 };
    context.device_type = tdpc0201_import();
    reset();
    reply(100, 1, 0, 0); /* Watchdog TICK before ACK. */
    reply(100, INPACKET_ACK, 0, 0);
    assert(context.device_type->set(&context) == 0 && read_count == 2);
    reset();
    EXPECT_ERROR(context.device_type->set(&context), ERROR_MSG_DEVICE_TIMEOUT);
    assert(now == 13000 && send_count == 1);
    free(context.device_type);

    context.device_type = tdsn5200_import();
    reset();
    EXPECT_ERROR(context.device_type->get(&context), ERROR_MSG_DEVICE_TIMEOUT);
    assert(now == 13000);
    reset();
    reply(1000, 0, 0, INPACKET_DUMP); /* Existing realignment read shares the deadline. */
    reply(1500, 0, 0, 0);
    EXPECT_ERROR(context.device_type->get(&context), ERROR_MSG_DEVICE_TIMEOUT);
    assert(now == 13000 && read_count == 3 && lengths[1] == 3);
    assert(timeouts[0] == 3000 && timeouts[1] == 2000 && timeouts[2] == 500);
    reset();
    reply(100, INPACKET_DUMP, 0, 0);
    assert(context.device_type->get(&context) == 0);
    free(context.device_type);

    td_device_t* (*listeners[])(void) = {
        tdfa30604_import, tdfa30608_import, tdfa60220_import, tdfa60250_import,
        tdpc0201_import, tdsn450_import, tdsn0700_import, tdsn5200_import,
        tdsn604xx_import, tdsn7360_import, tdsn7400_import, tdfa60228_import
    };
    for (unsigned int i = 0; i < sizeof(listeners) / sizeof(listeners[0]); i++) {
        memset(&context, 0, sizeof(context));
        context.device_type = listeners[i]();
        context.format = FORMAT_RAW;
        reset();
        if (i == 2 || i == 3) {
            reply(0, INPACKET_DEVREG, 1, 0); /* ADC calibration before listen. */
            replies[0].payload[3] = 1;
        }
        if (i == 6) {
            const uint8_t addresses[] = { 1, 2, 3, 4, 5 };
            for (unsigned int j = 0; j < sizeof(addresses); j++) {
                reply(0, INPACKET_DEVREG, addresses[j], 0);
                replies[j].payload[3] = 1;
            }
        }
        int setup_reads = reply_count;
        reply(10000, i < 2 ? 0x01 : INPACKET_DUMP, listeners[i] == tdfa60228_import ? 8 : 0, 0); /* An event after more than 3 seconds. */
        assert(context.device_type->listen(&context) == 0);
        assert(read_count == setup_reads + 1 && timeouts[setup_reads] == 0 && now == 20000);
        for (int j = 0; j < setup_reads; j++) assert(timeouts[j] == DEFAULT_TIMEOUT);
        if (listeners[i] == tdfa60228_import) {
            assert(context.device_type->input_report_size == 64);
            for (int j = 0; j < read_count; j++) assert(lengths[j] == 65);
        }
        free(context.device_type);
    }
}

int main(void)
{
    td_device_t device = { 0 };
    device.input_report_size = device.output_report_size = 8;
    td_context_t context = { 0 };
    context.device_type = &device;
    test_commands(&context);
    device.input_report_size = device.output_report_size = 64;
    test_commands(&context);
    test_devices();
    puts("PASS: 8/64-byte response deadlines, retries, legacy ACKs, sensors and all 12 event listeners");
    return 0;
}
