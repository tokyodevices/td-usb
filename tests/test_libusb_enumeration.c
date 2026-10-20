/* USB discovery is mocked; no libusb library or physical device is used. */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <usb.h>
#include "../tdhid.h"

struct usb_dev_handle { int slot; };
static struct usb_bus bus;
static struct usb_device devices[2];
static struct usb_config_descriptor configs[2];
static usb_dev_handle handles[2] = { {0}, {1} };
static const char *products[2];
static int failed_string, discovery_failure, claim_failures;
static int opens, closes, claims, detaches, live_handles;

void usb_init(void) { }
int usb_find_busses(void) { return discovery_failure == 1 ? -EIO : 0; }
int usb_find_devices(void)
{
    assert(discovery_failure != 1);
    return discovery_failure == 2 ? -EIO : 0;
}
struct usb_bus *usb_get_busses(void) { assert(!discovery_failure); return &bus; }
char *usb_strerror(void) { return "simulated failure"; }
usb_dev_handle *usb_open(struct usb_device *device)
{
    ptrdiff_t slot = device - devices;
    assert(slot >= 0 && slot < 2 && !(live_handles & (1 << slot)));
    live_handles |= 1 << slot;
    opens++;
    return &handles[slot];
}
int usb_close(usb_dev_handle *handle)
{
    assert(live_handles & (1 << handle->slot));
    live_handles &= ~(1 << handle->slot);
    closes++;
    return 0;
}
int usb_get_string_simple(usb_dev_handle *handle, int index, char *buffer, size_t size)
{
    assert(index == 1 || index == 2);
    if (handle->slot == 0 && index == failed_string) return -EIO;
    const char *value = index == 1 ? products[handle->slot] : (handle->slot ? "2222" : "1111");
    assert(strlen(value) + 1 <= size);
    strcpy(buffer, value);
    return (int)strlen(value);
}
int usb_claim_interface(usb_dev_handle *handle, int interface)
{
    assert(interface == 0);
    claims++;
    if (handle->slot == 0 && claim_failures > 0) { claim_failures--; return -EBUSY; }
    return 0;
}
int usb_detach_kernel_driver_np(usb_dev_handle *handle, int interface)
{
    assert(handle->slot == 0 && interface == 0);
    detaches++;
    return 0;
}
int usb_control_msg(usb_dev_handle *handle, int type, int request, int value,
                    int index, char *buffer, int size, int timeout)
{
    (void)handle; (void)type; (void)value; (void)index; (void)buffer; (void)size; (void)timeout;
    assert(request == USB_REQ_GET_DESCRIPTOR); /* The failed string's fallback. */
    return -EIO;
}
int usb_interrupt_read(usb_dev_handle *handle, int ep, char *buffer, int size, int timeout)
{
    (void)handle; (void)ep; (void)buffer; (void)size; (void)timeout;
    assert(!"Unexpected Input report read");
    return -EIO;
}

static void reset(void)
{
    assert(live_handles == 0);
    memset(&bus, 0, sizeof(bus));
    memset(devices, 0, sizeof(devices));
    memset(configs, 0, sizeof(configs));
    bus.devices = &devices[0];
    devices[0].next = &devices[1];
    for (int i = 0; i < 2; i++) {
        devices[i].descriptor.idVendor = 0x16c0;
        devices[i].descriptor.idProduct = 0x05df;
        devices[i].descriptor.iProduct = 1;
        devices[i].descriptor.iSerialNumber = 2;
        devices[i].descriptor.bNumConfigurations = 1;
        devices[i].config = &configs[i];
        configs[i].bNumInterfaces = 1;
    }
    products[0] = "IWS660"; products[1] = "IWT120";
    failed_string = discovery_failure = claim_failures = 0;
    opens = closes = claims = detaches = 0;
}

static void expect_open(int slot, const char *serial)
{
    int *handle = TdHidOpenDevice(0x16c0, 0x05df, "IWT120", (char *)serial);
    assert(handle == (int *)&handles[slot]);
    TdHidCloseDevice(handle);
    assert(opens == closes && live_handles == 0);
}

int main(void)
{
    char output[32];
    reset();
    assert(TdHidListDevices(0x16c0, 0x05df, "IWT120", NULL, 0) == 5);
    assert(TdHidListDevices(0x16c0, 0x05df, "IWT120", output, sizeof(output)) == 5);
    assert(strcmp(output, "2222") == 0 && opens == closes && claims == 0);
    expect_open(1, "2222");

    /* Both product and serial read failures must exclude the first device. */
    for (int failure = 1; failure <= 2; failure++) {
        reset();
        failed_string = failure;
        if (failure == 2) products[0] = "IWT120";
        expect_open(1, failure == 2 ? "2222" : NULL);
        assert(TdHidListDevices(0x16c0, 0x05df, "IWT120", output, sizeof(output)) == 5);
        assert(strcmp(output, "2222") == 0 && opens == closes);
    }

    /* Missing descriptors or interfaces must never return an unclaimed handle. */
    for (int invalid = 0; invalid < 3; invalid++) {
        reset();
        devices[0].next = NULL;
        products[0] = "IWT120";
        if (invalid == 0) devices[0].config = NULL;
        if (invalid == 1) configs[0].bNumInterfaces = 0;
        if (invalid == 2) devices[0].descriptor.bNumConfigurations = 0;
        assert(TdHidOpenDevice(0x16c0, 0x05df, NULL, NULL) == NULL);
        assert(opens == 1 && closes == 1 && claims == 0);
    }

    reset();
    products[0] = "IWT120";
    claim_failures = 1; /* Kernel driver detach followed by a successful claim. */
    expect_open(0, NULL);
    assert(claims == 2 && detaches == 1);
    reset();
    products[0] = "IWT120";
    claim_failures = 2; /* Close a busy candidate and try the next device. */
    expect_open(1, NULL);
    assert(claims == 3 && detaches == 1);

    for (int failure = 1; failure <= 2; failure++) {
        reset();
        discovery_failure = failure;
        strcpy(output, "stale");
        assert(TdHidListDevices(0x16c0, 0x05df, NULL, output, sizeof(output)) < 0);
        assert(output[0] == 0);
        assert(TdHidOpenDevice(0x16c0, 0x05df, NULL, NULL) == NULL);
        assert(opens == 0 && closes == 0);
    }
    reset();
    puts("PASS: Linux identity filtering, descriptor/claim cleanup and discovery errors");
    return 0;
}
