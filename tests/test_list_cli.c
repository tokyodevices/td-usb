/* Run the real list command with scripted discovery and allocation failure. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../td-usb.h"

static const char* scenario;
static int allocations, list_calls;

static void* test_malloc(size_t size)
{
    allocations++;
    /* The context is the first allocation, the list buffer is the second. */
    if (allocations == 2 && strcmp(scenario, "allocation-failure") == 0) return NULL;
    return malloc(size);
}

#define malloc test_malloc
#include "../td-usb.c"
#undef malloc

td_device_t* import_device_type(const char* model_name, int length)
{
    (void)length;
    scenario = model_name;
    td_device_t* device = calloc(1, sizeof(*device));
    assert(device);
    return device;
}
void delete_device_type(td_device_t* device) { free(device); }

int TdHidListDevices(uint16_t vendor, uint16_t product, const char* name, char* buffer, int capacity)
{
    (void)vendor; (void)product; (void)name;
    list_calls++;
    assert(list_calls <= 2); /* No implicit retry on a changing device list. */
    if (list_calls == 1) {
        assert(buffer == NULL && capacity == 0);
        if (!strcmp(scenario, "empty") || !strcmp(scenario, "grow")) return 1;
        return !strcmp(scenario, "shrink") ? 10 : 5;
    }
    assert(buffer != NULL && strcmp(scenario, "allocation-failure") != 0);
    if (!strcmp(scenario, "grow")) {
        assert(capacity == 1);
        buffer[0] = 0;
        return 5;
    }
    if (!strcmp(scenario, "empty")) { buffer[0] = 0; return 1; }
    assert(capacity >= 5);
    strcpy(buffer, "1111");
    return 5;
}

int* TdHidOpenDevice(uint16_t vendor, uint16_t product, const char* name, char* serial)
{
    (void)vendor; (void)product; (void)name; (void)serial;
    assert(!"List must not open a device for commands");
    return NULL;
}
void TdHidCloseDevice(int* handle) { (void)handle; assert(!"Unexpected close"); }
int TdTimer_Start(void callback(void*), void* parameter, int interval)
{
    (void)callback; (void)parameter; (void)interval;
    assert(!"List must not start a timer");
    return 0;
}
