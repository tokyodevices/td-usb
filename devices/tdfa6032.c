/**
* @file tdfa6032.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID						0x1778

#define REPORT_SIZE						8

#define REGNAME_INIT_DAC_VALUE			"INIT_DAC_VALUE"
#define REGNAME_DAC_VALUE				"DAC_VALUE"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"

#define ADDR_INIT_DAC_VALUE				0x01
#define ADDR_DAC_VALUE					0xF0
#define ADDR_FIRMWARE_VERSION			0xF2


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_INIT_DAC_VALUE) == 0) return ADDR_INIT_DAC_VALUE;
	else if (strcmp(name, REGNAME_DAC_VALUE) == 0) return ADDR_DAC_VALUE;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return ADDR_FIRMWARE_VERSION;

	fprintf(stderr, "Unknown device register name: %s\n", name);
	throw_exception(EXITCODE_INVALID_OPTION, NULL);

	return 0xFFFF;
}


static int set(td_context_t* context)
{
	uint16_t addr;
	uint32_t value;
	char* p;

	if (context->c == 0) throw_exception(EXITCODE_INVALID_OPTION, "No option is specified.");

	p = strchr(context->v[0], '=');

	if (p == NULL)
	{
		addr = ADDR_DAC_VALUE;
		value = atoi(context->v[0]);
	}
	else
	{
		*p = '\0';
		addr = devreg_name2addr(context->v[0]);
		value = atoi(p + 1);
	}

	tddev2_write_devreg(context, addr, value);

	return 0;
}


static int get(td_context_t* context)
{
	uint16_t addr = (context->c == 0) ? ADDR_DAC_VALUE : devreg_name2addr(context->v[0]);
	uint32_t value = tddev2_read_devreg(context, addr);
	
	if (context->format == FORMAT_RAW)
	{
		printf("%d", value);
	}
	else if (context->format == FORMAT_SIMPLE)
	{
		if (addr == ADDR_FIRMWARE_VERSION)
		{
			printf("%d.%d", (value >> 8), (value & 0xFF));
		}
		else
		{
			printf("%d", value);
		}
	}
	else
	{
		throw_exception(EXITCODE_INVALID_FORMAT, ERROR_MSG_INVALID_FORMAT);
	}

	printf("\n");
	fflush(stdout);

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
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdfa6032_import)(void) = export_type;
