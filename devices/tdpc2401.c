/**
* @file tdpc2401.c
* @author Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include "../td-usb.h"
#include "../tddevice.h"

#define PRODUCT_ID                      0x1789
#define REPORT_SIZE                     8

#define REGNAME_INITIAL_BRIDGE_STATUS   "INITIAL_BRIDGE_STATUS"
#define REGNAME_AUTO_ENABLE             "AUTO_ENABLE"
#define REGNAME_AUTO_MASK               "AUTO_MASK"
#define REGNAME_CONNECTED_TIME          "CONNECTED_TIME"
#define REGNAME_DISCONNECTED_TIME       "DISCONNECTED_TIME"
#define REGNAME_BRIDGE_STATUS           "BRIDGE_STATUS"
#define REGNAME_TIMER_REMAINING         "TIMER_REMAINING"
#define REGNAME_FIRMWARE_VERSION        "FIRMWARE_VERSION"

static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_INITIAL_BRIDGE_STATUS) == 0) return 0x01;
	else if (strcmp(name, REGNAME_AUTO_ENABLE) == 0) return 0x02;
	else if (strcmp(name, REGNAME_AUTO_MASK) == 0) return 0x03;
	else if (strcmp(name, REGNAME_CONNECTED_TIME) == 0) return 0x04;
	else if (strcmp(name, REGNAME_DISCONNECTED_TIME) == 0) return 0x05;
	else if (strcmp(name, REGNAME_BRIDGE_STATUS) == 0) return 0x06;
	else if (strcmp(name, REGNAME_TIMER_REMAINING) == 0) return 0x07;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return 0xF2;

	fprintf(stderr, "Unknown device register name: %s\n", name);
	throw_exception(EXITCODE_INVALID_OPTION, NULL);
	return 0xFFFF;
}

static int set(td_context_t* context)
{
	if (context->c == 0) throw_exception(EXITCODE_INVALID_OPTION, "No option is specified.");

	for (int i = 0; i < context->c; i++)
	{
		uint16_t addr = 0x06;
		char* number = context->v[i];
		char* p = strchr(number, '=');
		if (p != NULL)
		{
			*p = '\0';
			addr = devreg_name2addr(number);
			number = p + 1;
		}

		uint32_t minimum = 0, maximum = 3;
		switch (addr)
		{
		case 0x02: maximum = 1; break;
		case 0x03: minimum = 1; break;
		case 0x04:
		case 0x05: minimum = 1; maximum = UINT32_MAX; break;
		case 0x07:
		case 0xF2: throw_exception(EXITCODE_INVALID_OPTION, "Device register is read-only.");
		}

		if (*number < '0' || *number > '9')
			throw_exception(EXITCODE_INVALID_OPTION, "Invalid unsigned decimal value.");
		errno = 0;
		char* end;
		unsigned long value = strtoul(number, &end, 10);
		if (*end != '\0') throw_exception(EXITCODE_INVALID_OPTION, "Invalid unsigned decimal value.");
		if (errno == ERANGE || value < minimum || value > maximum)
			throw_exception(EXITCODE_INVALID_RANGE, "Value is out of range.");

		tddev2_write_devreg(context, addr, (uint32_t)value);
	}
	return 0;
}

static int get(td_context_t* context)
{
	if (context->format != FORMAT_RAW && context->format != FORMAT_SIMPLE)
		throw_exception(EXITCODE_INVALID_FORMAT, ERROR_MSG_INVALID_FORMAT);

	if (context->c == 0)
	{
		context->c = 1;
		context->v[0] = REGNAME_BRIDGE_STATUS;
	}

	for (int i = 0; i < context->c; i++)
	{
		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = tddev2_read_devreg(context, addr);
		if (i > 0) printf(",");
		printf("%" PRIu32, value);
	}
	printf("\n");
	fflush(stdout);
	return 0;
}

static int init(td_context_t* context)
{
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_AUTO_ENABLE), 0);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_INITIAL_BRIDGE_STATUS), 3);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_AUTO_MASK), 3);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_CONNECTED_TIME), 3595);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_DISCONNECTED_TIME), 5);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_BRIDGE_STATUS), 3);
	printf("Device has been Initialized.\n");
	return 0;
}

static td_device_t* export_type(void)
{
	td_device_t* device = (td_device_t*)malloc(sizeof(td_device_t));
	memset(device, 0, sizeof(td_device_t));
	device->vendor_id = TOKYODEVICES_VENDOR_ID;
	device->product_id = PRODUCT_ID;
	device->output_report_size = REPORT_SIZE;
	device->input_report_size = REPORT_SIZE;
	device->get = get;
	device->set = set;
	device->save = tddev2_save_to_flash;
	device->init = init;
	device->destroy = tddev2_destroy_firmware;
	return device;
}

td_device_t* (*tdpc2401_import)(void) = export_type;
