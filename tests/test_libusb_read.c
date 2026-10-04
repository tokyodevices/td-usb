#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <usb.h>
#include "../tdhid.h"

static int read_result, expected_timeout;

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
    puts("PASS: Linux Input reads, timeout forwarding, disconnect and short reports");
    return 0;
}
