/**
* @file tdsn7400.c
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

#define PRODUCT_ID						0x1780

#define REPORT_SIZE						8

#define REGNAME_TEMPERATURE				"TEMPERATURE"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"
#define REGNAME_MODEL_ID				"MODEL_ID"

#define ADDR_TEMPERATURE				0xF0
#define ADDR_FIRMWARE_VERSION			0xF2
#define ADDR_MODEL_ID					0xF3


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_TEMPERATURE) == 0) return ADDR_TEMPERATURE;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return ADDR_FIRMWARE_VERSION;
	else if (strcmp(name, REGNAME_MODEL_ID) == 0) return ADDR_MODEL_ID;

	fprintf(stderr, "Unknown device register name: %s\n", name);
	throw_exception(EXITCODE_INVALID_OPTION, NULL);

	return 0xFFFF;
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
			fprintf(stderr, "Invalid option: %s\n", context->v[i]);
			throw_exception(EXITCODE_INVALID_OPTION, NULL);
		}

		*p = '\0';

		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = atoi(p + 1);

		tddev2_write_devreg(context, addr, value);
	}

	return 0;
}

static int get(td_context_t* context)
{
	if (context->c == 0)
	{
		context->c = 1;
		context->v[0] = REGNAME_TEMPERATURE;
	}

	uint16_t addr = devreg_name2addr(context->v[0]);
	uint32_t value = tddev2_read_devreg(context, addr);

	if (context->format == FORMAT_RAW)
	{
		printf("%d", value);
	}
	else if (context->format == FORMAT_SIMPLE)
	{
		if (addr == ADDR_TEMPERATURE)
		{
			value = value & 0xFFFFE000;
			printf("%f", (double)(((double)value / 1048576.0)));
		}
		else if (addr == ADDR_FIRMWARE_VERSION)
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


static int listen(td_context_t* context)
{
	uint8_t buffer[REPORT_SIZE + 1];

	while (1)
	{
		if ((TdHidListenReport(context->handle, buffer, REPORT_SIZE + 1)) != TDHID_SUCCESS)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
		if (buffer[1] == INPACKET_DUMP) break;
	}

	uint32_t value = (buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];

	if (context->format == FORMAT_RAW)
	{
		printf("%d", value);
	}
	else if (context->format == FORMAT_SIMPLE)
	{
		value = value & 0xFFFFE000;
		printf("%f", (double)(((double)value / 1048576.0)));
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
	device->listen = listen;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdsn7400_import)(void) = export_type;
