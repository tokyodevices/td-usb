/* TDPW6620: current control and two-point user calibration (firmware >=0.10.0). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "../td-usb.h"
#include "../tddevice.h"
#include "../tdhid.h"

static const struct { const char *name; uint16_t address; } registers[] = {
    {"ISET_MA", 0}, {"IOUT_MA", 2}, {"STATUS", 3}, {"FAULT", 4},
    {"COMMAND_RESULT", 6}, {"CAL_100_MA", 0x10}, {"CAL_700_MA", 0x11},
    {"FIRMWARE_VERSION", 0xF2}
};

static uint16_t address_of(const char *name)
{
    for (unsigned i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i)
        if (strcmp(name, registers[i].name) == 0) return registers[i].address;
    throw_exception(EXITCODE_INVALID_OPTION, "Unknown TDPW6620 register.");
    return 0xFFFF;
}

/* Decimal mA -> integer microamps, without binary floating-point rounding. */
static uint32_t parse_ma(const char *text)
{
    uint32_t whole = 0, fraction = 0, scale = 100;
    if (*text < '0' || *text > '9')
        throw_exception(EXITCODE_INVALID_OPTION, "Expected decimal mA, up to three fractional digits.");
    while (*text >= '0' && *text <= '9') {
        whole = whole * 10 + (uint32_t)(*text++ - '0');
        if (whole > 1000) throw_exception(EXITCODE_INVALID_RANGE, "Current is out of range.");
    }
    if (*text == '.') {
        ++text;
        if (*text < '0' || *text > '9')
            throw_exception(EXITCODE_INVALID_OPTION, "Missing fractional digits.");
        while (*text >= '0' && *text <= '9') {
            if (scale == 0) throw_exception(EXITCODE_INVALID_OPTION, "Use at most three fractional digits.");
            fraction += (uint32_t)(*text++ - '0') * scale;
            scale /= 10;
        }
    }
    if (*text != '\0') throw_exception(EXITCODE_INVALID_OPTION, "Invalid decimal mA.");
    return whole * 1000 + fraction;
}

static void check_result(td_context_t *context)
{
    uint32_t result = tddev2_read_devreg(context, 6);
    if (result == 0) return;
    static const char *messages[] = {
        "", "RANGE: calibration/current value rejected.", "ADDRESS: register unsupported.",
        "READ_ONLY: register cannot be written.",
        "NOT_READY: check ISET, output/fault status, settling, and calibration point order.",
        "STORAGE: calibration could not be saved."
    };
    throw_exception(result == 1 ? EXITCODE_INVALID_RANGE : EXITCODE_DEVICE_IO_ERROR,
                    result < sizeof(messages) / sizeof(messages[0]) ? messages[result] : "Unknown command result.");
}

static int set(void *opaque)
{
    td_context_t *context = opaque;
    if (context->c == 0) throw_exception(EXITCODE_INVALID_OPTION, "No register specified.");
    for (int i = 0; i < context->c; ++i) {
        char *name = context->v[i], *number = strchr(name, '=');
        if (number == NULL) throw_exception(EXITCODE_INVALID_OPTION, "Use NAME=value.");
        *number++ = '\0';
        uint16_t address = address_of(name);
        if (address != 0 && address != 0x10 && address != 0x11)
            throw_exception(EXITCODE_INVALID_OPTION, "Register is read-only.");
        uint32_t value = parse_ma(number);
        if (address == 0) {
            if (value % 1000 != 0 || (value != 0 && (value < 100000 || value > 1000000)))
                throw_exception(EXITCODE_INVALID_RANGE, "ISET_MA must be 0 or an integer from 100 to 1000.");
            value /= 1000;
        } else if (value < (address == 0x10 ? 90000U : 650000U) ||
                   value > (address == 0x10 ? 110000U : 750000U)) {
            throw_exception(EXITCODE_INVALID_RANGE, "Calibration reading is outside its allowed range.");
        }
        tddev2_write_devreg(context, address, value);
        check_result(context);
        if (tddev2_read_devreg(context, address) != value)
            throw_exception(EXITCODE_DEVICE_IO_ERROR, "Applied register value does not match the request.");
    }
    return 0;
}

static int get(void *opaque)
{
    td_context_t *context = opaque;
    if (context->format != FORMAT_RAW && context->format != FORMAT_SIMPLE)
        throw_exception(EXITCODE_INVALID_FORMAT, ERROR_MSG_INVALID_FORMAT);
    if (context->c == 0) { context->c = 1; context->v[0] = "IOUT_MA"; }
    for (int i = 0; i < context->c; ++i) {
        uint16_t address = address_of(context->v[i]);
        uint32_t value = tddev2_read_devreg(context, address);
        if (i != 0) printf(",");
        if (address == 0x10 || address == 0x11) printf("%" PRIu32 ".%03" PRIu32, value / 1000, value % 1000);
        else if (address == 2) printf("%" PRId32, (int32_t)value);
        else printf("%" PRIu32, value);
    }
    printf("\n");
    fflush(stdout);
    return 0;
}

static int save(void *opaque)
{
    td_context_t *context = opaque;
    uint8_t report[MAX_REPORT_LENGTH + 1] = {0, OUTPACKET_SAVE, 0x50};
    const uint8_t expected[] = {INPACKET_ACK, OUTPACKET_SAVE};
    if (TdHidSetReport(context->handle, report, sizeof(report), USB_HID_REPORT_TYPE_OUTPUT))
        throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
    int result = tddev2_wait_response(context, report, expected, sizeof(expected));
    if (result != TDHID_SUCCESS)
        throw_exception(EXITCODE_DEVICE_IO_ERROR, result == TDHID_ERR_TIMEOUT ? ERROR_MSG_DEVICE_TIMEOUT : ERROR_MSG_DEVICE_IO_ERROR);
    check_result(context); /* ACK alone must not print a successful-save message. */
    puts("Calibration saved. Output is OFF.");
    return 0;
}

static td_device_t *export_type(void)
{
    td_device_t *device = (td_device_t *)calloc(1, sizeof(*device));
    if (device == NULL) throw_exception(EXITCODE_DEVICE_IO_ERROR, "Out of memory.");
    device->vendor_id = TOKYODEVICES_VENDOR_ID;
    device->product_id = 0x178A;
    device->input_report_size = device->output_report_size = 64;
    device->get = get;
    device->set = set;
    device->save = save;
    return device;
}

td_device_t *(*tdpw6620_import)(void) = export_type;
