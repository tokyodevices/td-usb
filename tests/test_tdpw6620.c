#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../td-usb.h"
#include "../tddevice.h"
#include "../tdhid.h"

extern td_device_t *(*tdpw6620_import)(void);
static jmp_buf failure;
static int expected_error, writes, saves;
static uint16_t written_address;
static uint32_t written_value, command_result;

void throw_exception(int code, const char *message)
{
    (void)message;
    assert(code == expected_error && code != 0);
    longjmp(failure, 1);
}
uint32_t tddev2_read_devreg(td_context_t *context, uint16_t address)
{
    (void)context;
    return address == 6 ? command_result : written_value;
}
int tddev2_write_devreg(td_context_t *context, uint16_t address, uint32_t value)
{
    (void)context;
    ++writes;
    written_address = address;
    written_value = value;
    return 0;
}
int TdHidSetReport(int *handle, unsigned char *data, int length, uint8_t type)
{
    (void)handle;
    assert(length == 65 && type == USB_HID_REPORT_TYPE_OUTPUT);
    assert(data[0] == 0 && data[1] == 0xF1 && data[2] == 0x50);
    ++saves;
    return 0;
}
int tddev2_wait_response(td_context_t *context, uint8_t *report, const uint8_t *expected, int length)
{
    (void)context; (void)report;
    assert(length == 2 && expected[0] == 0xFF && expected[1] == 0xF1);
    return TDHID_SUCCESS;
}

static void set(td_context_t *context, const char *argument, int error, uint16_t address, uint32_t value)
{
    char input[128];
    strcpy(input, argument);
    context->c = 1; context->v[0] = input;
    expected_error = error;
    writes = 0;
    if (setjmp(failure) == 0) {
        assert(context->device_type->set(context) == 0);
        assert(error == 0);
    }
    if (!error) assert(writes == 1 && written_address == address && written_value == value);
    else if (!command_result) assert(writes == 0);
}

int main(void)
{
    td_context_t context = {0};
    context.device_type = tdpw6620_import();
    assert(context.device_type->product_id == 0x178A);
    assert(context.device_type->input_report_size == 64 && context.device_type->output_report_size == 64);
    set(&context, "ISET_MA=100", 0, 0, 100);
    set(&context, "CAL_100_MA=98.601", 0, 0x10, 98601);
    set(&context, "ISET_MA=700", 0, 0, 700);
    set(&context, "CAL_700_MA=694.3", 0, 0x11, 694300);
    set(&context, "ISET_MA=0", 0, 0, 0);
    const char *invalid[] = {"CAL_100_MA=nan", "CAL_100_MA=98.6001", "CAL_100_MA=-100",
        "CAL_700_MA=700.", "CAL_START=1", "IOUT_MA=100"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        set(&context, invalid[i], EXITCODE_INVALID_OPTION, 0, 0);
    set(&context, "CAL_100_MA=100000000000000000000", EXITCODE_INVALID_RANGE, 0, 0);
    set(&context, "ISET_MA=100.001", EXITCODE_INVALID_RANGE, 0, 0);
    command_result = 4;
    set(&context, "CAL_100_MA=99.8", EXITCODE_DEVICE_IO_ERROR, 0, 0);
    expected_error = EXITCODE_DEVICE_IO_ERROR;
    if (setjmp(failure) == 0) { context.device_type->save(&context); assert(!"Rejected SAVE reported success"); }
    command_result = 0;
    expected_error = 0;
    assert(context.device_type->save(&context) == 0 && saves == 2);
    free(context.device_type);
    puts("PASS: calibration decimal values, rejection results and SAVE protocol");
    return 0;
}
