/**
* @file tdsn7360.c
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

#define PRODUCT_ID						0x1781

#define REPORT_SIZE						64

#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"

#define ADDR_FIRMWARE_VERSION			0xF2


static int skip_count = 0;

static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return ADDR_FIRMWARE_VERSION;

	fprintf(stderr, "Unknown device register name: %s\n", name);
	throw_exception(EXITCODE_INVALID_OPTION, NULL);

	return 0xFFFF;
}


static int get(td_context_t* context)
{
	if (context->c == 0)
	{
		context->c = 1;
		context->v[0] = REGNAME_FIRMWARE_VERSION;
	}

	uint16_t addr = devreg_name2addr(context->v[0]);
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


static int listen(td_context_t* context)
{
	uint8_t buffer[REPORT_SIZE + 1];

	while (1)
	{
		if ((TdHidListenReport(context->handle, buffer, REPORT_SIZE + 1)) != TDHID_SUCCESS)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
		if (buffer[1] == INPACKET_DUMP) break;
	}

	for (int i = 0; i < buffer[2]; i++)
	{
		if (skip_count != context->skip)
		{
			skip_count++;
			continue;
		}

		skip_count = 0;

		int16_t x = (buffer[i*6+4] << 8) | buffer[i*6+3];
		int16_t y = (buffer[i*6+6] << 8) | buffer[i*6+5];
		int16_t z = (buffer[i*6+8] << 8) | buffer[i*6+7];

		if (context->format == FORMAT_RAW)
		{
			printf("%d,%d,%d\n", x,y,z);
		}
		else
		{
			printf("%f,%f,%f\n",
				(double)x * (2.5 / 32767.0),
				(double)y * (2.5 / 32767.0),
				(double)z * (2.5 / 32767.0));
		}
	}

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
	device->listen = listen;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdsn7360_import)(void) = export_type;
