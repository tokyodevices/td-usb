/**
* @file tdsn450.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
* @date 2023-8-25
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"


#define TDSN450_PRODUCT_ID		0x1776

#define FEATURE_REPORT_SIZE		16
#define IN_REPORT_SIZE			1

static int write(td_context_t* context)
{
	uint16_t threshold_value;
	uint8_t report_buffer[FEATURE_REPORT_SIZE + 1];

	if (context->c != 1)
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Only one value can be set.");
	}

	threshold_value = atoi(context->v[0]);

	if (threshold_value > 1024)
	{
		throw_exception(EXITCODE_INVALID_RANGE, "Value must be smaller than 1024.");
	}

	memset(report_buffer, 0, FEATURE_REPORT_SIZE + 1);
	report_buffer[0] = 0x00;
	report_buffer[1] = 0x89; // Update Threshold Opcode
	report_buffer[2] = (threshold_value & 0xFF);
	report_buffer[3] = (threshold_value >> 8);

	if (TdHidSetReport(context->handle, report_buffer, FEATURE_REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	return 0;
}


static int read(td_context_t* context)
{	
	uint8_t buffer[FEATURE_REPORT_SIZE + 1];
	memset(buffer, 0, FEATURE_REPORT_SIZE + 1);
	if (TdHidGetReport(context->handle, buffer, FEATURE_REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");


	uint16_t value = (buffer[4] << 8) | buffer[3];
	uint8_t state = buffer[5];
	uint16_t threshold = (buffer[8] << 8) | buffer[7];

	if (context->c == 0)
	{
		printf("%d\n", value);
		/*
		printf("value: %d\n", value);
		printf("threshold: %d\n", threshold);
		printf("state: %d\n", state);
		*/
		fflush(stdout);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Unknown option.");
	}

	return 0;
}


static int listen(td_context_t* context)
{
	unsigned char buffer[IN_REPORT_SIZE + 1];
	memset(buffer, 0, IN_REPORT_SIZE + 1);

	if (TdHidListenReport(context->handle, buffer, IN_REPORT_SIZE + 1) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	if (context->format == FORMAT_RAW || context->format == FORMAT_SIMPLE)
	{
		printf("%d\n", buffer[1]); // print out new state.
		fflush(stdout);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_FORMAT, "Unknown format");
	}

	return 0;
}


static td_device_t* export_type(void)
{
	td_device_t* dt = (td_device_t*)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->vendor_id = TOKYODEVICES_VENDOR_ID;
	dt->product_id = TDSN450_PRODUCT_ID;
	dt->output_report_size = FEATURE_REPORT_SIZE;
	dt->listen = listen;
	dt->init = tddev1_init_operation;
	dt->set = write;
	dt->get = read;

	return dt;
}

td_device_t* (*tdsn450_import)(void) = export_type;
