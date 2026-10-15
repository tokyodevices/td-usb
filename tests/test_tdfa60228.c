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
#include "../tdhid.h"

/* Host-only device callback tests. Register values and DUMPs follow firmware
 * devreg.h and ProcessMeasurements(): CH0..7, 12 bits, 64-byte reports. */
extern td_device_t* (*tdfa60228_import)(void);
static uint32_t registers[2][256];
static uint16_t read_addresses[128], write_addresses[32];
static uint32_t write_values[32];
static int read_count, write_count, save_count, destroy_count, fail_write;
static int other_handle;
static struct { uint8_t bytes[65]; int result; } packets[16];
static int packet_count, packet_reads;
static jmp_buf error_jump;
static int expected_error, error_seen;
static const char* error_message;
static const uint16_t adc_values[8] = {0, 400, 1200, 2400, 400, 1000, 2048, 4095};
static const uint32_t calibrations[8] = {1000, 2000, 3000, 4000, 500, 1000, 2000, 4095};
static const char* raw_row = "0,400,1200,2400,400,1000,2048,4095\n";
static const char* current_row = "0.000000,5.000000,10.000000,15.000000,20.000000,25.000000,25.600000,25.000000\n";

void throw_exception(int code, const char* message)
{
    if (!expected_error || code != expected_error)
        fprintf(stderr, "Unexpected error %d: %s\n", code, message ? message : "");
    assert(expected_error != 0 && code == expected_error);
    error_seen = 1;
    error_message = message;
    longjmp(error_jump, 1);
}

uint32_t tddev2_read_devreg(td_context_t* context, uint16_t addr)
{
    assert(addr < 256 && read_count < 128);
    read_addresses[read_count++] = addr;
    return registers[context->handle == &other_handle][addr];
}

int tddev2_write_devreg(td_context_t* context, uint16_t addr, uint32_t value)
{
    assert(addr < 256 && write_count < 32);
    write_addresses[write_count] = addr;
    write_values[write_count++] = value;
    registers[context->handle == &other_handle][addr] = value;
    if (fail_write) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_TIMEOUT);
    return 0;
}

int tddev2_save_to_flash(td_context_t* context) { (void)context; save_count++; return 0; }
int tddev2_destroy_firmware(td_context_t* context) { (void)context; destroy_count++; return 0; }

int TdHidReadInputReport(int* handle, unsigned char* buffer, int length, int timeout_ms)
{
    (void)handle;
    assert(length == 65 && timeout_ms == 0 && packet_reads < packet_count);
    memcpy(buffer, packets[packet_reads].bytes, 65);
    return packets[packet_reads++].result;
}

static void reset_io(void)
{
    read_count = write_count = packet_reads = packet_count = fail_write = 0;
    memset(packets, 0, sizeof(packets));
}

static void reset_registers(void)
{
    memset(registers, 0, sizeof(registers));
    for (int bank = 0; bank < 2; bank++) {
        registers[bank][0] = 4;
        registers[bank][0xF2] = 258;
        for (int channel = 0; channel < 8; channel++) {
            registers[bank][1 + channel] = calibrations[channel];
            registers[bank][0x80 + channel] = adc_values[channel];
        }
    }
}

static td_context_t new_context(void)
{
    td_context_t context = {0};
    context.device_type = tdfa60228_import();
    return context;
}

static void invoke(td_context_t* context, int (*operation)(void*), const char* expected, int error)
{
    FILE* capture = tmpfile();
    assert(capture != NULL);
    fflush(stdout);
    int saved_stdout = dup(fileno(stdout));
    assert(saved_stdout >= 0 && dup2(fileno(capture), fileno(stdout)) >= 0);
    expected_error = error;
    error_seen = 0;
    error_message = NULL;
    if (setjmp(error_jump) == 0) assert(operation(context) == 0);
    assert(error_seen == (error != 0));
    expected_error = 0;
    fflush(stdout);
    assert(dup2(saved_stdout, fileno(stdout)) >= 0);
    close(saved_stdout);
    rewind(capture);
    char output[2048] = {0};
    size_t size = fread(output, 1, sizeof(output) - 1, capture);
    assert(size < sizeof(output) - 1);
    fclose(capture);
    size_t length = 0;
    for (size_t i = 0; i < size; i++) if (output[i] != '\r') output[length++] = output[i];
    output[length] = '\0';
    if (strcmp(output, expected) != 0) fprintf(stderr, "Expected: %sActual: %s\n", expected, output);
    assert(strcmp(output, expected) == 0);
}

static void one_argument(td_context_t* context, const char* argument)
{
    context->c = 1;
    context->v[0] = (char*)argument;
}

static void dump(const uint16_t* values)
{
    assert(packet_count < 16);
    uint8_t* bytes = packets[packet_count++].bytes;
    bytes[1] = INPACKET_DUMP;
    bytes[2] = 8;
    for (int channel = 0; channel < 8; channel++) {
        bytes[3 + channel * 2] = (uint8_t)values[channel];
        bytes[4 + channel * 2] = (uint8_t)(values[channel] >> 8);
    }
}

static void test_get(void)
{
    td_context_t context = new_context();
    context.format = FORMAT_RAW;
    reset_io();
    invoke(&context, context.device_type->get, raw_row, 0);
    assert(context.c == 0 && read_count == 8);
    for (int i = 0; i < 8; i++) assert(read_addresses[i] == 0x80 + i);
    reset_io();
    invoke(&context, context.device_type->get, raw_row, 0);
    assert(read_count == 8); /* get --loop reuses the same arguments. */

    context.format = FORMAT_SIMPLE;
    reset_io();
    invoke(&context, context.device_type->get, current_row, 0);
    assert(read_count == 16);
    for (int i = 0; i < 8; i++)
        assert(read_addresses[i * 2] == 0x80 + i && read_addresses[i * 2 + 1] == 1 + i);
    reset_io();
    invoke(&context, context.device_type->get, current_row, 0);
    assert(read_count == 8);

    char* names[] = {"ADC_VALUE_CH7", "CONTROL_FLAG", "CALIBRATION_CH0", "FIRMWARE_VERSION", "ADC_VALUE_CH0"};
    context.c = 5;
    for (int i = 0; i < 5; i++) context.v[i] = names[i];
    invoke(&context, context.device_type->get, "25.000000,4,1000,1.2,0.000000\n", 0);
    context.format = FORMAT_RAW;
    invoke(&context, context.device_type->get, "4095,4,1000,258,0\n", 0);
    for (int i = 0; i < 8; i++) {
        char name[32], expected[32];
        sprintf(name, "CALIBRATION_CH%d", i);
        sprintf(expected, "%u\n", (unsigned int)calibrations[i]);
        one_argument(&context, name);
        reset_io();
        invoke(&context, context.device_type->get, expected, 0);
        assert(read_count == 1 && read_addresses[0] == i + 1);
    }
    /* All 16 argument slots and repeated names preserve the requested order. */
    context.c = TD_CONTEXT_MAX_ARG_COUNT;
    for (int i = 0; i < context.c; i++) context.v[i] = "CONTROL_FLAG";
    reset_io();
    invoke(&context, context.device_type->get, "4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4\n", 0);
    assert(read_count == 16);

    const char* invalid[] = {"ADC_VALUE", "CALIBRATION", "ADC_VALUE_CH8", "ADC_VALUE_CH-1",
        "ADC_VALUE_CH00", "ADC_VALUE_CH0x", "CALIBRATION_CH8", "CALIBRATION_CH", "", "UNKNOWN"};
    for (unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        one_argument(&context, invalid[i]);
        reset_io();
        invoke(&context, context.device_type->get, "", EXITCODE_INVALID_OPTION);
        assert(read_count == 0);
    }
    context.c = 2; context.v[0] = "CONTROL_FLAG"; context.v[1] = "UNKNOWN";
    reset_io();
    invoke(&context, context.device_type->get, "", EXITCODE_INVALID_OPTION);
    assert(read_count == 0);
    one_argument(&context, "ADC_VALUE_CH0");
    registers[0][0x80] = 4096;
    invoke(&context, context.device_type->get, "", EXITCODE_DEVICE_IO_ERROR);
    reset_registers();
    free(context.device_type);
}

static void test_invalid_adc(void)
{
    const char* unavailable =
        "TDFA60228 ADC measurement unavailable (not yet measured or ADC communication failed).";
    const uint8_t formats[] = {FORMAT_SIMPLE, FORMAT_RAW};
    const uint32_t invalid[] = {0xFFFF, 4096, 0xFFFE, 0x10000, UINT32_MAX};
    for (unsigned int format = 0; format < sizeof(formats) / sizeof(formats[0]); format++) {
        td_context_t context = new_context();
        uint16_t samples[8];
        context.format = formats[format];
        for (int channel = 0; channel < 8; channel++) {
            registers[0][0x80 + channel] = 0xFFFF;
            samples[channel] = 0xFFFF;
        }
        /* Firmware returns all-channel 0xFFFF before measurement or on ADC failure. */
        reset_io();
        invoke(&context, context.device_type->get, "", EXITCODE_DEVICE_IO_ERROR);
        assert(strcmp(error_message, unavailable) == 0);
        dump(samples);
        invoke(&context, context.device_type->listen, "", EXITCODE_DEVICE_IO_ERROR);
        assert(strcmp(error_message, unavailable) == 0 && packet_reads == 1);

        reset_registers();
        for (unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
            /* A bad last channel must not leave a partial row on stdout. */
            registers[0][0x87] = invalid[i];
            reset_io();
            invoke(&context, context.device_type->get, "", EXITCODE_DEVICE_IO_ERROR);
            assert(strcmp(error_message, invalid[i] == 0xFFFF ? unavailable
                : "Invalid TDFA60228 ADC value.") == 0);
            if (invalid[i] <= UINT16_MAX) {
                memcpy(samples, adc_values, sizeof(samples));
                samples[7] = (uint16_t)invalid[i];
                one_argument(&context, "ADC_VALUE_CH0");
                context.skip = 1;
                reset_io(); dump(samples); dump(adc_values);
                invoke(&context, context.device_type->listen, "", EXITCODE_DEVICE_IO_ERROR);
                assert(strcmp(error_message, invalid[i] == 0xFFFF ? unavailable
                    : "Invalid TDFA60228 DUMP ADC value.") == 0);
                /* Check all channels before skipping, including unselected CH7. */
                assert(packet_reads == 1);
                context.c = 0;
                context.skip = 0;
            }
        }
        free(context.device_type);
        reset_registers();
    }
}

static void check_set(td_context_t* context, const char* argument, int error, uint16_t addr, uint32_t value)
{
    one_argument(context, argument); /* Read-only literals also verify argv is not modified. */
    reset_io();
    invoke(context, context->device_type->set, "", error);
    if (error) assert(write_count == 0);
    else assert(write_count == 1 && write_addresses[0] == addr && write_values[0] == value);
}

static void test_set(void)
{
    td_context_t context = new_context();
    check_set(&context, "CONTROL_FLAG=0", 0, 0, 0);
    check_set(&context, "CONTROL_FLAG=4", 0, 0, 4);
    for (int i = 0; i < 8; i++) {
        char argument[40];
        sprintf(argument, "CALIBRATION_CH%d=1", i);
        check_set(&context, argument, 0, (uint16_t)(1 + i), 1);
        sprintf(argument, "CALIBRATION_CH%d=4095", i);
        check_set(&context, argument, 0, (uint16_t)(1 + i), 4095);
    }
    const char* invalid[] = {"", "4", "=1", "UNKNOWN=1", "CONTROL_FLAG=", "CONTROL_FLAG=-1",
        "CONTROL_FLAG=+4", "CONTROL_FLAG= 4", "CONTROL_FLAG=4 ", "CONTROL_FLAG=4x", "CONTROL_FLAG=4=0",
        "CONTROL_FLAG=0x04", "CALIBRATION_CH0=1.5", "CALIBRATION=3103", "CALIBRATION_CH8=1", "FIRMWARE_VERSION=1"};
    for (unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
        check_set(&context, invalid[i], EXITCODE_INVALID_OPTION, 0, 0);
    for (int i = 0; i < 8; i++) {
        char argument[32];
        sprintf(argument, "ADC_VALUE_CH%d=0", i);
        check_set(&context, argument, EXITCODE_INVALID_OPTION, 0, 0);
    }
    const char* ranges[] = {"CONTROL_FLAG=1", "CONTROL_FLAG=2", "CONTROL_FLAG=5", "CONTROL_FLAG=255",
        "CALIBRATION_CH0=0", "CALIBRATION_CH7=4096", "CALIBRATION_CH0=4294967295",
        "CALIBRATION_CH0=4294967296", "CALIBRATION_CH0=18446744073709551616"};
    for (unsigned int i = 0; i < sizeof(ranges) / sizeof(ranges[0]); i++)
        check_set(&context, ranges[i], EXITCODE_INVALID_RANGE, 0, 0);
    context.c = 0;
    reset_io();
    invoke(&context, context.device_type->set, "", EXITCODE_INVALID_OPTION);
    assert(write_count == 0);

    context.c = 3;
    context.v[0] = "CONTROL_FLAG=0"; context.v[1] = "CALIBRATION_CH7=1234"; context.v[2] = "CONTROL_FLAG=4";
    reset_io();
    invoke(&context, context.device_type->set, "", 0);
    assert(write_count == 3 && write_addresses[0] == 0 && write_values[0] == 0);
    assert(write_addresses[1] == 8 && write_values[1] == 1234 && write_addresses[2] == 0 && write_values[2] == 4);
    reset_io();
    invoke(&context, context.device_type->set, "", 0); /* Reusable arguments, including SET loop. */
    context.v[2] = "CALIBRATION_CH0=0";
    reset_io();
    invoke(&context, context.device_type->set, "", EXITCODE_INVALID_RANGE);
    assert(write_count == 0); /* Validate later arguments before any write. */
    free(context.device_type);
    reset_registers();
}

static void test_calibration_lifetime(void)
{
    td_context_t context = new_context();
    one_argument(&context, "ADC_VALUE_CH1");
    reset_io();
    invoke(&context, context.device_type->get, "5.000000\n", 0);
    assert(read_count == 2);
    check_set(&context, "CALIBRATION_CH1=1000", 0, 2, 1000);
    one_argument(&context, "ADC_VALUE_CH1");
    reset_io();
    invoke(&context, context.device_type->get, "10.000000\n", 0);
    assert(read_count == 2);

    one_argument(&context, "CALIBRATION_CH1=500");
    reset_io(); fail_write = 1;
    invoke(&context, context.device_type->set, "", EXITCODE_DEVICE_IO_ERROR);
    fail_write = 0;
    one_argument(&context, "ADC_VALUE_CH1");
    reset_io();
    invoke(&context, context.device_type->get, "20.000000\n", 0);
    assert(read_count == 2); /* SET may execute even if its ACK is lost. */

    context.handle = &other_handle;
    reset_io();
    invoke(&context, context.device_type->get, "5.000000\n", 0);
    assert(read_count == 2);
    td_context_t second = new_context();
    one_argument(&second, "ADC_VALUE_CH1");
    reset_io();
    invoke(&second, second.device_type->get, "20.000000\n", 0);
    assert(read_count == 2);
    free(second.device_type);
    free(context.device_type);

    const uint32_t invalid[] = {0, 4096, UINT32_MAX};
    for (unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        context = new_context();
        registers[0][8] = invalid[i];
        one_argument(&context, "ADC_VALUE_CH7");
        reset_io();
        invoke(&context, context.device_type->get, "", EXITCODE_DEVICE_IO_ERROR);
        reset_io();
        invoke(&context, context.device_type->listen, "", EXITCODE_DEVICE_IO_ERROR);
        assert(packet_reads == 0);
        context.format = FORMAT_RAW;
        reset_io();
        invoke(&context, context.device_type->get, "4095\n", 0);
        assert(read_count == 1); /* Raw data does not require calibration. */
        dump(adc_values);
        invoke(&context, context.device_type->listen, "4095\n", 0);
        assert(read_count == 1);
        free(context.device_type);
    }
    reset_registers();
}

static void test_listen(void)
{
    td_context_t context = new_context();
    context.format = FORMAT_RAW;
    reset_io();
    packets[packet_count++].bytes[1] = INPACKET_ACK;
    dump(adc_values);
    invoke(&context, context.device_type->listen, raw_row, 0);
    assert(read_count == 0 && write_count == 0 && packet_reads == 2);
    context.format = FORMAT_SIMPLE;
    reset_io(); dump(adc_values);
    invoke(&context, context.device_type->listen, current_row, 0);
    assert(read_count == 8 && packet_reads == 1);
    reset_io(); dump(adc_values);
    invoke(&context, context.device_type->listen, current_row, 0);
    assert(read_count == 0);
    context.c = 3; context.v[0] = "ADC_VALUE_CH7"; context.v[1] = "ADC_VALUE_CH0"; context.v[2] = "ADC_VALUE_CH7";
    reset_io(); dump(adc_values);
    invoke(&context, context.device_type->listen, "25.000000,0.000000,25.000000\n", 0);

    context.c = 0; context.format = FORMAT_RAW; context.skip = 2;
    const uint16_t discarded[8] = {1,2,3,4,5,6,7,8};
    reset_io(); dump(discarded); dump(discarded); dump(adc_values);
    invoke(&context, context.device_type->listen, raw_row, 0);
    assert(packet_reads == 3); /* One-shot skips frames, never individual channels. */
    reset_io(); dump(discarded); dump(discarded); dump(adc_values);
    invoke(&context, context.device_type->listen, raw_row, 0);
    assert(packet_reads == 3); /* The same cadence applies to listen --loop. */
    context.skip = 0;

    const int bad_counts[] = {0, 1, 7, 9, 31, 32, 255};
    for (unsigned int i = 0; i < sizeof(bad_counts) / sizeof(bad_counts[0]); i++) {
        reset_io(); dump(adc_values); packets[0].bytes[2] = (uint8_t)bad_counts[i];
        invoke(&context, context.device_type->listen, "", EXITCODE_DEVICE_IO_ERROR);
        assert(packet_reads == 1);
    }
    reset_io(); dump(adc_values); packets[0].bytes[18] = 0x10;
    invoke(&context, context.device_type->listen, "", EXITCODE_DEVICE_IO_ERROR);
    for (int error = TDHID_ERR_IO; error <= TDHID_ERR_TIMEOUT; error++) {
        reset_io(); dump(adc_values); packets[0].result = error;
        invoke(&context, context.device_type->listen, "", EXITCODE_DEVICE_IO_ERROR);
    }
    const char* invalid[] = {"CONTROL_FLAG", "FIRMWARE_VERSION", "CALIBRATION_CH0", "ADC_VALUE_CH8"};
    for (unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        one_argument(&context, invalid[i]); reset_io();
        invoke(&context, context.device_type->listen, "", EXITCODE_INVALID_OPTION);
        assert(read_count == 0 && packet_reads == 0);
    }
    context.c = 0; context.skip = -1;
    invoke(&context, context.device_type->listen, "", EXITCODE_INVALID_OPTION);
    context.skip = 0;
    for (int format = FORMAT_JSON; format <= FORMAT_TSV; format++) {
        context.format = (uint8_t)format; reset_io();
        invoke(&context, context.device_type->get, "", EXITCODE_INVALID_FORMAT);
        invoke(&context, context.device_type->listen, "", EXITCODE_INVALID_FORMAT);
        assert(read_count == 0 && packet_reads == 0);
    }
    assert(context.device_type->save(&context) == 0 && save_count == 1);
    assert(context.device_type->destroy(&context) == 0 && destroy_count == 1);
    free(context.device_type);
}

int main(void)
{
    td_device_t* device = tdfa60228_import();
    assert(device->vendor_id == 0x32EE && device->product_id == 0x1788);
    assert(device->input_report_size == 64 && device->output_report_size == 64);
    assert(device->get && device->set && device->listen && device->save && device->destroy && !device->init);
    free(device);
    reset_registers();
    test_get();
    test_invalid_adc();
    test_set();
    test_calibration_lifetime();
    test_listen();
    puts("PASS: TDFA60228 eight-channel registers, calibration, ADC errors, DUMP rows and frame skipping");
    return 0;
}
