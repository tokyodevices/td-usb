/**
* @file tdsn7300.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID						0x1785

#define IN_REPORT_BUFFER_SIZE			16U
#define OUT_REPORT_BUFFER_SIZE			8U

#define OUTPACKET_MEASURE				0x80
#define OUTPACKET_SETLED				0x83
#define INPACKET_DATA					0x80

uint8_t in_buffer[IN_REPORT_BUFFER_SIZE + 1];
uint8_t out_buffer[OUT_REPORT_BUFFER_SIZE + 1];

static int set(td_context_t* context)
{
	if (context->c == 0) throw_exception(EXITCODE_INVALID_OPTION, "No value is specified.");

	memset(out_buffer, 0, OUT_REPORT_BUFFER_SIZE + 1);
	out_buffer[0] = 0x00;
	out_buffer[1] = OUTPACKET_SETLED;
	out_buffer[2] = atoi(context->v[0]);
	int result = TdHidSetReport(context->handle, out_buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT);
	if (result != TDHID_SUCCESS) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	return 0;
}

static int get(td_context_t* context)
{
	memset(out_buffer, 0, OUT_REPORT_BUFFER_SIZE + 1);
	out_buffer[0] = 0x00;
	out_buffer[1] = OUTPACKET_MEASURE;
	int result = TdHidSetReport(context->handle, out_buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT);
	if (result != TDHID_SUCCESS) throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	while (1)
	{
		if ((TdHidListenReport(context->handle, in_buffer, IN_REPORT_BUFFER_SIZE + 1)) != TDHID_SUCCESS)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
		if (in_buffer[1] == INPACKET_DATA) break;
	}

	if (context->c == 0)
	{
		int co2_ppm = (int)((in_buffer[3] << 8) | in_buffer[4]);
		double t_degC = -45.0 + 175.0 * ((double)((in_buffer[6] << 8) | in_buffer[7]) / 65535.0);
		double rh_pRH = 100.0 * ((double)((in_buffer[9] << 8) | in_buffer[10]) / 65535.0);
		if (rh_pRH > 100.0) rh_pRH = 100.0;
		if (rh_pRH < 0.0) rh_pRH = 0.0;
		printf("%d,%.1f,%.1f\n", co2_ppm, t_degC, rh_pRH);
	}
	else if (strcmp(context->v[0], "LED_STATUS") == 0)
	{
		printf("%d\n", in_buffer[2]);
	}
	else if (strcmp(context->v[0], "FIRMWARE_VERSION") == 0)
	{
		printf("%d.%d\n", in_buffer[14], in_buffer[13]);
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
	device->output_report_size = OUT_REPORT_BUFFER_SIZE;
	device->input_report_size = IN_REPORT_BUFFER_SIZE;
	device->get = get;
	device->set = set;

	return device;
}

td_device_t* (*tdsn7300_import)(void) = export_type;
