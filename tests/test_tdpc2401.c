#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define close _close
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include "../td-usb.h"
#include "../tddevice.h"

/* Exercise the device callbacks with register I/O recorded instead of USB access. */
extern td_device_t* (*tdpc2401_import)(void);
static uint32_t registers[256];
static uint16_t write_addresses[16];
static uint32_t write_values[16];
static int write_count, save_count, expected_error;
static jmp_buf error_jump;

void throw_exception(int code, const char* message)
{
    (void)message;
    assert(expected_error != 0 && code == expected_error);
    longjmp(error_jump, 1);
}

uint32_t tddev2_read_devreg(td_context_t* context, uint16_t addr)
{
    (void)context;
    assert(addr < 256);
    return registers[addr];
}

int tddev2_write_devreg(td_context_t* context, uint16_t addr, uint32_t value)
{
    (void)context;
    assert(write_count < 16);
    write_addresses[write_count] = addr;
    write_values[write_count++] = value;
    return 0;
}

int tddev2_save_to_flash(td_context_t* context) { (void)context; save_count++; return 0; }

static void check_set(td_context_t* context, const char* argument, int error, uint16_t addr, uint32_t value)
{
    char buffer[128];
    strcpy(buffer, argument);
    context->c = 1;
    context->v[0] = buffer;
    write_count = 0;
    expected_error = error;
    if (setjmp(error_jump) == 0) {
        assert(context->device_type->set(context) == 0);
        assert(error == 0);
    }
    if (error) assert(write_count == 0);
    else assert(write_count == 1 && write_addresses[0] == addr && write_values[0] == value);
    expected_error = 0;
}

static void check_get(td_context_t* context, const char* expected)
{
    char output[256] = {0};
    FILE* capture = tmpfile();
    assert(capture != NULL);
    fflush(stdout);
    int saved_stdout = dup(fileno(stdout));
    assert(saved_stdout >= 0 && dup2(fileno(capture), fileno(stdout)) >= 0);
    assert(context->device_type->get(context) == 0);
    fflush(stdout);
    assert(dup2(saved_stdout, fileno(stdout)) >= 0);
    close(saved_stdout);
    rewind(capture);
    assert(fgets(output, sizeof(output), capture) != NULL);
    assert(strcmp(output, expected) == 0);
    fclose(capture);
}

int main(void)
{
    td_context_t context = {0};
    td_device_t* device = tdpc2401_import();
    context.device_type = device;
    assert(device->vendor_id == 0x32EE && device->product_id == 0x1789);
    assert(device->input_report_size == 8 && device->output_report_size == 8);
    assert(device->save != NULL && device->destroy == NULL);
    assert(device->save(&context) == 0 && save_count == 1);
    assert(device->listen == NULL);

    check_set(&context, "0", 0, 6, 0);
    check_set(&context, "3", 0, 6, 3);
    check_set(&context, "BRIDGE_STATUS=2", 0, 6, 2);
    check_set(&context, "INITIAL_BRIDGE_STATUS=0", 0, 1, 0);
    check_set(&context, "INITIAL_BRIDGE_STATUS=3", 0, 1, 3);
    check_set(&context, "AUTO_MASK=1", 0, 3, 1);
    check_set(&context, "AUTO_MASK=3", 0, 3, 3);
    check_set(&context, "CONNECTED_TIME=1", 0, 4, 1);
    check_set(&context, "CONNECTED_TIME=2147483648", 0, 4, 2147483648U);
    check_set(&context, "CONNECTED_TIME=4294967295", 0, 4, UINT32_MAX);
    check_set(&context, "DISCONNECTED_TIME=4294967295", 0, 5, UINT32_MAX);

    const char* invalid[] = {"", "-1", "+1", " 1", "abc", "1x", "=1", "UNKNOWN=1",
        "BRIDGE_STATUS=", "BRIDGE_STATUS=1=2", "CONNECTED_TIME=0x10",
        "TIMER_REMAINING=1", "FIRMWARE_VERSION=258"};
    for (unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
        check_set(&context, invalid[i], EXITCODE_INVALID_OPTION, 0, 0);
    const char* out_of_range[] = {"4", "INITIAL_BRIDGE_STATUS=4", "AUTO_ENABLE=2",
        "AUTO_MASK=0", "AUTO_MASK=4", "CONNECTED_TIME=0", "DISCONNECTED_TIME=0",
        "CONNECTED_TIME=4294967296", "CONNECTED_TIME=18446744073709551616"};
    for (unsigned int i = 0; i < sizeof(out_of_range) / sizeof(out_of_range[0]); i++)
        check_set(&context, out_of_range[i], EXITCODE_INVALID_RANGE, 0, 0);

    context.c = 0;
    expected_error = EXITCODE_INVALID_OPTION;
    if (setjmp(error_jump) == 0) { device->set(&context); assert(!"Missing SET arguments accepted"); }
    expected_error = 0;

    char options[][40] = {"AUTO_ENABLE=0", "AUTO_MASK=2", "CONNECTED_TIME=10",
        "DISCONNECTED_TIME=5", "AUTO_ENABLE=1"};
    const uint16_t addresses[] = {2, 3, 4, 5, 2};
    const uint32_t values[] = {0, 2, 10, 5, 1};
    context.c = 5;
    for (int i = 0; i < 5; i++) context.v[i] = options[i];
    write_count = 0;
    assert(device->set(&context) == 0 && write_count == 5);
    for (int i = 0; i < 5; i++)
        assert(write_addresses[i] == addresses[i] && write_values[i] == values[i]);

    registers[1] = 3; registers[2] = 1; registers[3] = 2;
    registers[4] = 2147483648U; registers[5] = UINT32_MAX;
    registers[6] = 3; registers[7] = UINT32_MAX; registers[0xF2] = 258;
    context.c = 0;
    check_get(&context, "3\n");
    check_get(&context, "3\n"); /* The same context is reused by get --loop. */
    char* names[] = {"INITIAL_BRIDGE_STATUS", "AUTO_ENABLE", "AUTO_MASK", "CONNECTED_TIME",
        "DISCONNECTED_TIME", "BRIDGE_STATUS", "TIMER_REMAINING", "FIRMWARE_VERSION"};
    context.c = 8;
    for (int i = 0; i < 8; i++) context.v[i] = names[i];
    check_get(&context, "3,1,2,2147483648,4294967295,3,4294967295,258\n");
    context.format = FORMAT_RAW;
    check_get(&context, "3,1,2,2147483648,4294967295,3,4294967295,258\n");
    context.format = FORMAT_JSON;
    expected_error = EXITCODE_INVALID_FORMAT;
    if (setjmp(error_jump) == 0) { device->get(&context); assert(!"Invalid format accepted"); }
    expected_error = 0;

    write_count = 0;
    const uint16_t init_addresses[] = {2, 1, 3, 4, 5, 6};
    const uint32_t init_values[] = {0, 3, 3, 3595, 5, 3};
    assert(device->init(&context) == 0 && write_count == 6 && save_count == 1);
    for (int i = 0; i < 6; i++)
        assert(write_addresses[i] == init_addresses[i] && write_values[i] == init_values[i]);
    free(device);
    puts("PASS: TDPC2401 registers, uint32 boundaries, validation, command order, output and init");
    return 0;
}
