#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <usb.h>
#include "../tdhid.h"

static int read_result, expected_timeout;
static int control_result;

int __wrap_usb_control_msg(usb_dev_handle* handle, int type, int request, int value,
    int index, char* bytes, int size, int timeout)
{
    (void)handle;
    assert(type == (USB_TYPE_CLASS | USB_RECIP_DEVICE | USB_ENDPOINT_IN));
    assert(request == USBRQ_HID_GET_REPORT && value == (USB_HID_REPORT_TYPE_FEATURE << 8));
    assert(index == 0 && size == 16 && timeout == DEFAULT_TIMEOUT);
    if (control_result > 0) memset(bytes, 0x42, control_result);
    return control_result;
}

int __wrap_usb_interrupt_read(usb_dev_handle* handle, int ep, char* bytes, int size, int timeout)
{
    (void)handle;
    assert(ep == 0x81 && size == 8 && timeout == expected_timeout);
    if (read_result > 0) memset(bytes, 0xF6, read_result);
    return read_result;
}

int main(void)
{
    unsigned char report[9] = { 0 };
    int timeouts[] = { 0, 37, 3000 };
    for (unsigned int i = 0; i < sizeof(timeouts) / sizeof(timeouts[0]); i++) {
        expected_timeout = timeouts[i];
        read_result = 8;
        assert(TdHidReadInputReport(NULL, report, 9, expected_timeout) == TDHID_SUCCESS);
        assert(report[0] == 0 && report[1] == 0xF6);
        read_result = -ETIMEDOUT;
        assert(TdHidReadInputReport(NULL, report, 9, expected_timeout) == TDHID_ERR_TIMEOUT);
        read_result = -ENODEV;
        assert(TdHidReadInputReport(NULL, report, 9, expected_timeout) == TDHID_ERR_IO);
        read_result = 1;
        assert(TdHidReadInputReport(NULL, report, 9, expected_timeout) == TDHID_ERR_IO);
        read_result = 0;
        assert(TdHidReadInputReport(NULL, report, 9, expected_timeout) == TDHID_ERR_IO);
    }
    unsigned char feature[17];
    int control_results[] = { 16, 15, 1, 0, -ETIMEDOUT, -ENODEV };
    for (unsigned int i = 0; i < sizeof(control_results) / sizeof(control_results[0]); i++) {
        memset(feature, 0xA5, sizeof(feature));
        control_result = control_results[i];
        int result = TdHidGetReport(NULL, feature, sizeof(feature), USB_HID_REPORT_TYPE_FEATURE);
        assert(result == (control_result == 16 ? USBOPEN_SUCCESS : USBOPEN_ERR_IO));
        assert(feature[0] == 0);
    }
    assert(TdHidGetReport(NULL, feature, 1, USB_HID_REPORT_TYPE_FEATURE) == USBOPEN_ERR_IO);
    puts("PASS: Linux Input/Feature reads, timeouts, disconnect and short reports");
    return 0;
}
