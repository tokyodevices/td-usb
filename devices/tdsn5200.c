/**
* @file tdsn5200.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID						0x177C

#define REPORT_SIZE						16


#define OUTPACKET_START					8
#define OUTPACKET_STOP					9
#define OUTPACKET_DUMP					10
#define OUTPACKET_RSPAD_CAL				20
#define OUTPACKET_XTALK_CAL				21
#define OUTPACKET_ZD_OFFSET_CAL			22


static uint8_t buffer[REPORT_SIZE + 1];
static int is_start = 0;


static void start(td_context_t* context, uint8_t oneshot)
{
	if (is_start == 0)
	{
		memset(buffer, 0, REPORT_SIZE + 1);
		buffer[0] = 0x00;
		buffer[1] = OUTPACKET_START;
		buffer[2] = oneshot; // continuous or one-shot
		int result = TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT);
		if (result != TDHID_SUCCESS) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

		is_start = 1;
	}
}

static void print(td_context_t* context)
{
	int num_obj = buffer[2];
	int s1 = buffer[3];
	int d1 = (int)((buffer[5] << 8) | buffer[4]);
	int s2 = buffer[6];
	int d2 = (int)((buffer[8] << 8) | buffer[7]);
	int s3 = buffer[9];
	int d3 = (int)((buffer[11] << 8) | buffer[10]);
	int s4 = buffer[12];
	int d4 = (int)((buffer[14] << 8) | buffer[13]);

	if (context->format == FORMAT_RAW)
	{
		printf("%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			num_obj, s1, d1, s2, d2, s3, d3, s4, d4);
	}
	else
	{
		if (num_obj == 0)
		{
			printf("65535\n");
		}
		else
		{
			if (s1 != 0) d1 = 65535;
			if (s2 != 0) d2 = 65535;
			if (s3 != 0) d3 = 65535;
			if (s4 != 0) d4 = 65535;

			int dmin = (int)fmin(fmin(fmin(d1, d2), d3), d4);

			printf("%d\n", dmin);
		}
	}

	fflush(stdout);
}

static int get(td_context_t* context)
{
	start(context, 1);

	memset(buffer, 0, REPORT_SIZE + 1);
	buffer[0] = 0x00;
	buffer[1] = OUTPACKET_DUMP;
	int result = TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT);
	if (result != TDHID_SUCCESS) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	while (1)
	{
		if ((TdHidListenReport(context->handle, buffer, REPORT_SIZE + 1)) != TDHID_SUCCESS)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
		if (buffer[1] == INPACKET_DUMP) break;
	}

	print(context);

	return 0;
}


static int listen(td_context_t* context)
{
	start(context, 0);

	while (1)
	{
		if ((TdHidListenReport(context->handle, buffer, REPORT_SIZE + 1)) != TDHID_SUCCESS)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
		if (buffer[1] == INPACKET_DUMP) break;
	}

	print(context);

	return 0;
}


static int init(td_context_t* context)
{
	uint8_t buffer[REPORT_SIZE + 1];
	int result;

	if (context->c != 1)
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Invalid options.");
	}

	if ( strcmp(context->v[0], "spad") == 0 )
	{
		printf("Performing RefSPAD calibration.\n");

		memset(buffer, 0, REPORT_SIZE + 1);
		buffer[0] = 0x00;
		buffer[1] = OUTPACKET_RSPAD_CAL;
		result = TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT);
		if (result != TDHID_SUCCESS) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
	}
	else if ( strcmp(context->v[0], "xtalk") == 0 )
	{
		printf("Performing Xtalk calibration.\n");

		memset(buffer, 0, REPORT_SIZE + 1);
		buffer[0] = 0x00;
		buffer[1] = OUTPACKET_XTALK_CAL;
		result = TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT);
		if (result != TDHID_SUCCESS) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
	}
	else if (strcmp(context->v[0], "offset") == 0)
	{
		printf("Performing offset calibration.\n");

		memset(buffer, 0, REPORT_SIZE + 1);
		buffer[0] = 0x00;
		buffer[1] = OUTPACKET_ZD_OFFSET_CAL;
		result = TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT);
		if (result != TDHID_SUCCESS) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Invalid options.");
	}

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
	device->init = init;
	device->get = get;
	device->listen = listen;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdsn5200_import)(void) = export_type;
