/**
* @file tdfa60228.c
* @author Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID          0x1788
#define REPORT_SIZE         64
#define CHANNEL_COUNT       8
#define ADDR_CONTROL_FLAG   0x00
#define ADDR_CALIBRATION    0x01
#define ADDR_ADC_VALUE      0x80
#define ADDR_FIRMWARE_VERSION 0xF2
#define CONTROL_DUMP_ENABLE 0x04
#define ADC_MAX             4095

/* The public device is first so delete_device_type() can free this allocation.
 * Calibration belongs to this device/handle, never to another serial number. */
typedef struct {
    td_device_t device;
    int* calibration_handle;
    uint32_t calibration[CHANNEL_COUNT];
    unsigned int calibration_valid;
} tdfa60228_device_t;

static int name_equals(const char* name, size_t length, const char* expected)
{
    return length == strlen(expected) && memcmp(name, expected, length) == 0;
}

static int named_channel(const char* name, size_t length, const char* prefix)
{
    size_t prefix_length = strlen(prefix);
    if (length == prefix_length + 1 && memcmp(name, prefix, prefix_length) == 0
        && name[prefix_length] >= '0' && name[prefix_length] < '0' + CHANNEL_COUNT)
        return name[prefix_length] - '0';
    return -1;
}

static uint16_t devreg_name2addr(const char* name, size_t length)
{
    if (name_equals(name, length, "CONTROL_FLAG")) return ADDR_CONTROL_FLAG;
    if (name_equals(name, length, "FIRMWARE_VERSION")) return ADDR_FIRMWARE_VERSION;
    int channel = named_channel(name, length, "CALIBRATION_CH");
    if (channel >= 0) return (uint16_t)(ADDR_CALIBRATION + channel);
    channel = named_channel(name, length, "ADC_VALUE_CH");
    if (channel >= 0) return (uint16_t)(ADDR_ADC_VALUE + channel);
    throw_exception(EXITCODE_INVALID_OPTION, "Unknown TDFA60228 register name.");
    return 0xFFFF;
}

static int adc_channel(uint16_t address)
{
    return address >= ADDR_ADC_VALUE && address < ADDR_ADC_VALUE + CHANNEL_COUNT
        ? address - ADDR_ADC_VALUE : -1;
}

static tdfa60228_device_t* device_state(td_context_t* context)
{
    tdfa60228_device_t* state = (tdfa60228_device_t*)context->device_type;
    if (state->calibration_handle != context->handle) {
        state->calibration_handle = context->handle;
        state->calibration_valid = 0;
    }
    return state;
}

static uint32_t calibration_value(td_context_t* context, int channel)
{
    tdfa60228_device_t* state = device_state(context);
    if (!(state->calibration_valid & (1U << channel))) {
        uint32_t value = tddev2_read_devreg(context, (uint16_t)(ADDR_CALIBRATION + channel));
        if (value == 0 || value > ADC_MAX)
            throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid TDFA60228 calibration value.");
        state->calibration[channel] = value;
        state->calibration_valid |= 1U << channel;
    }
    return state->calibration[channel];
}

static void validate_format(td_context_t* context)
{
    if (context->format != FORMAT_SIMPLE && context->format != FORMAT_RAW)
        throw_exception(EXITCODE_INVALID_FORMAT, ERROR_MSG_INVALID_FORMAT);
}

static int select_registers(td_context_t* context, uint16_t* addresses, int adc_only)
{
    if (context->c < 0 || context->c > TD_CONTEXT_MAX_ARG_COUNT)
        throw_exception(EXITCODE_INVALID_OPTION, "Invalid register count.");
    int count = context->c == 0 ? CHANNEL_COUNT : context->c;
    for (int i = 0; i < count; i++) {
        addresses[i] = context->c == 0 ? (uint16_t)(ADDR_ADC_VALUE + i)
            : devreg_name2addr(context->v[i], strlen(context->v[i]));
        if (adc_only && adc_channel(addresses[i]) < 0)
            throw_exception(EXITCODE_INVALID_OPTION, "Listen accepts only ADC_VALUE_CH0 through ADC_VALUE_CH7.");
    }
    return count;
}

static void print_values(td_context_t* context, const uint16_t* addresses, const uint32_t* values, int count)
{
    for (int i = 0; i < count; i++) {
        if (i > 0) printf(",");
        int channel = adc_channel(addresses[i]);
        if (context->format == FORMAT_SIMPLE && channel >= 0) {
            /* Calibration has already been loaded and checked before printing. */
            tdfa60228_device_t* state = device_state(context);
            printf("%.6f", 25.0 * values[i] / state->calibration[channel]);
        }
        else if (context->format == FORMAT_SIMPLE && addresses[i] == ADDR_FIRMWARE_VERSION)
            printf("%" PRIu32 ".%" PRIu32, values[i] >> 8, values[i] & 0xFF);
        else
            printf("%" PRIu32, values[i]);
    }
    printf("\n");
    fflush(stdout);
}

static int get(void* opaque)
{
    td_context_t* context = (td_context_t*)opaque;
    uint16_t addresses[TD_CONTEXT_MAX_ARG_COUNT];
    uint32_t values[TD_CONTEXT_MAX_ARG_COUNT];
    validate_format(context);
    int count = select_registers(context, addresses, 0);
    for (int i = 0; i < count; i++) {
        values[i] = tddev2_read_devreg(context, addresses[i]);
        int channel = adc_channel(addresses[i]);
        if (channel >= 0) {
            if (values[i] > ADC_MAX)
                throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid TDFA60228 ADC value.");
            if (context->format == FORMAT_SIMPLE) calibration_value(context, channel);
        }
    }
    print_values(context, addresses, values, count);
    return 0;
}

static int set(void* opaque)
{
    td_context_t* context = (td_context_t*)opaque;
    uint16_t addresses[TD_CONTEXT_MAX_ARG_COUNT];
    uint32_t values[TD_CONTEXT_MAX_ARG_COUNT];
    if (context->c <= 0 || context->c > TD_CONTEXT_MAX_ARG_COUNT)
        throw_exception(EXITCODE_INVALID_OPTION, "Specify one or more NAME=VALUE pairs.");

    /* Validate the complete request before writing, without modifying argv.
     * SET values are unsigned decimal, just like other register-based devices. */
    for (int i = 0; i < context->c; i++) {
        const char* argument = context->v[i];
        const char* separator = strchr(argument, '=');
        if (separator == NULL)
            throw_exception(EXITCODE_INVALID_OPTION, "Specify a NAME=VALUE pair.");
        addresses[i] = devreg_name2addr(argument, (size_t)(separator - argument));
        if (adc_channel(addresses[i]) >= 0 || addresses[i] == ADDR_FIRMWARE_VERSION)
            throw_exception(EXITCODE_INVALID_OPTION, "Device register is read-only.");
        const char* number = separator + 1;
        if (*number < '0' || *number > '9')
            throw_exception(EXITCODE_INVALID_OPTION, "Invalid unsigned decimal value.");
        errno = 0;
        char* end;
        unsigned long value = strtoul(number, &end, 10);
        if (*end != '\0') throw_exception(EXITCODE_INVALID_OPTION, "Invalid unsigned decimal value.");
        if (errno == ERANGE || value > UINT32_MAX
            || (addresses[i] == ADDR_CONTROL_FLAG ? (value != 0 && value != CONTROL_DUMP_ENABLE)
                : (value < 1 || value > ADC_MAX)))
            throw_exception(EXITCODE_INVALID_RANGE, "Value is out of range.");
        values[i] = (uint32_t)value;
    }

    tdfa60228_device_t* state = device_state(context);
    for (int i = 0; i < context->c; i++) {
        /* A failed acknowledgement must not leave a cached, possibly stale value. */
        if (addresses[i] >= ADDR_CALIBRATION && addresses[i] < ADDR_CALIBRATION + CHANNEL_COUNT)
            state->calibration_valid &= ~(1U << (addresses[i] - ADDR_CALIBRATION));
        tddev2_write_devreg(context, addresses[i], values[i]);
    }
    return 0;
}

static int listen(void* opaque)
{
    td_context_t* context = (td_context_t*)opaque;
    uint16_t addresses[TD_CONTEXT_MAX_ARG_COUNT];
    uint32_t values[TD_CONTEXT_MAX_ARG_COUNT];
    uint16_t samples[CHANNEL_COUNT];
    uint8_t buffer[REPORT_SIZE + 1];
    validate_format(context);
    int count = select_registers(context, addresses, 1);
    if (context->skip < 0) throw_exception(EXITCODE_INVALID_OPTION, "Invalid skip count.");
    if (context->format == FORMAT_SIMPLE)
        for (int i = 0; i < count; i++) calibration_value(context, adc_channel(addresses[i]));

    int skipped = 0;
    for (;;) {
        if (TdHidReadInputReport(context->handle, buffer, sizeof(buffer), 0) != TDHID_SUCCESS)
            throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
        if (buffer[1] != INPACKET_DUMP) continue;
        if (buffer[2] != CHANNEL_COUNT)
            throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid TDFA60228 DUMP channel count.");
        for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
            samples[channel] = (uint16_t)(buffer[3 + channel * 2] | ((uint16_t)buffer[4 + channel * 2] << 8));
            if (samples[channel] > ADC_MAX)
                throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid TDFA60228 DUMP ADC value.");
        }
        /* Skip complete scans; a one-shot listen still produces one full row. */
        if (skipped < context->skip) { skipped++; continue; }
        break;
    }
    for (int i = 0; i < count; i++) values[i] = samples[adc_channel(addresses[i])];
    print_values(context, addresses, values, count);
    return 0;
}

static int save(void* context) { return tddev2_save_to_flash((td_context_t*)context); }
static int destroy(void* context) { return tddev2_destroy_firmware((td_context_t*)context); }

static td_device_t* export_type(void)
{
    tdfa60228_device_t* state = (tdfa60228_device_t*)calloc(1, sizeof(tdfa60228_device_t));
    if (state == NULL) throw_exception(EXITCODE_DEVICE_IO_ERROR, "Could not allocate TDFA60228 state.");
    td_device_t* device = &state->device;
    device->vendor_id = TOKYODEVICES_VENDOR_ID;
    device->product_id = PRODUCT_ID;
    device->output_report_size = REPORT_SIZE;
    device->input_report_size = REPORT_SIZE;
    device->get = get;
    device->set = set;
    device->listen = listen;
    device->save = save;
    device->destroy = destroy;
    return device;
}

td_device_t* (*tdfa60228_import)(void) = export_type;
