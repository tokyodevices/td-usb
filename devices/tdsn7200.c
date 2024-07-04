/**
* @file tdsn7200.c
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

#define PRODUCT_ID						0x177d

#define IN_REPORT_BUFFER_SIZE			16U
#define OUT_REPORT_BUFFER_SIZE			8U

#define OUTPACKET_MEASURE				0x80
#define INPACKET_DATA					0x80

uint8_t in_buffer[IN_REPORT_BUFFER_SIZE + 1];
uint8_t out_buffer[OUT_REPORT_BUFFER_SIZE + 1];

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

	/*
	for (int i = 0; i < IN_REPORT_BUFFER_SIZE; i++)
		printf("%02x", in_buffer[i]);
	printf("\n");
	*/

	double t_degC = -45.0 + (175.0 * (double)(in_buffer[3] * 256 + in_buffer[2]))/65535.0;
	double rh_pRH = -6.0 + (125.0 * (double)(in_buffer[5] * 256 + in_buffer[4]))/65535.0;
	if (rh_pRH > 100.0) rh_pRH = 100.0;
	if (rh_pRH < 0.0) rh_pRH = 0.0;

	double pressure = (double)((in_buffer[8] << 16) | (in_buffer[7] << 8) | in_buffer[6]) / 4096.0;

	printf("%.1f,%.1f,%.1f\n", t_degC, rh_pRH, pressure);

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
	device->destroy = tddev2_destroy_firmware;

	return device;
}

td_device_t* (*tdsn7200_import)(void) = export_type;
