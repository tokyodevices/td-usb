/**
* @file tdfa30301.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID						0x1786

#define REPORT_SIZE						8

#define REGNAME_CONTROL_FLAG			"CONTROL_FLAG"
#define REGNAME_INITIAL_GPIO_STATUS		"INITIAL_GPIO_STATUS"
#define REGNAME_GPIO_STATUS				"GPIO_STATUS"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_CONTROL_FLAG) == 0) return 0x00;
	else if (strcmp(name, REGNAME_INITIAL_GPIO_STATUS) == 0) return 0x01;
	else if (strcmp(name, REGNAME_GPIO_STATUS) == 0) return 0x87;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return 0xF2;
	else
	{
		fprintf(stderr, "Unknown device register name: %s\n", name);
		throw_exception(EXITCODE_INVALID_OPTION, NULL);
		return 0xFFFF;
	}
}

static int set(td_context_t* context)
{
	char* p;

	if (context->c == 0) throw_exception(EXITCODE_INVALID_OPTION, "No option is specified.");

	for (int i = 0; i < context->c; i++)
	{
		p = strchr(context->v[i], '=');

		if (p == NULL)
		{
			uint32_t value = atoi(context->v[i]);
			tddev2_write_devreg(context, 0x87, value);
		}
		else
		{
			*p = '\0';
			uint16_t addr = devreg_name2addr(context->v[i]);
			uint32_t value = atoi(p + 1);
			tddev2_write_devreg(context, addr, value);
		}

	}

	return 0;
}


static int get(td_context_t* context)
{
	if (context->c == 0) // Default behavier of read operation
	{
		context->c = 1;
		context->v[0] = REGNAME_GPIO_STATUS;
	}

	for (int i = 0; i < context->c; i++)
	{
		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = tddev2_read_devreg(context, addr);

		if (context->format == FORMAT_RAW || context->format == FORMAT_SIMPLE)
		{
			if (i > 0) printf(",");
			printf("%d", value);
		}
		else
		{
			throw_exception(EXITCODE_INVALID_FORMAT, "Unknown format");
		}
	}

	printf("\n");
	fflush(stdout);

	return 0;
}


static int init(td_context_t* context)
{
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_INITIAL_GPIO_STATUS), 0);

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

	return device;
}


td_device_t* (*tdfa30301_import)(void) = export_type;
