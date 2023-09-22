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


#define TDSN450_PRODUCT_ID				0x1776

#define FEATURE_REPORT_SIZE				16
#define IN_REPORT_SIZE					1

#define REGNAME_INTENSITY				"INTENSITY"
#define REGNAME_THRESHOLD				"THRESHOLD"
#define REGNAME_HYSTERESIS				"HYSTERESIS"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"


static uint8_t report_buffer[FEATURE_REPORT_SIZE + 1];


static int write(td_context_t* context)
{
	memset(report_buffer, 0, FEATURE_REPORT_SIZE + 1);

	if (context->c != 1) throw_exception(EXITCODE_INVALID_OPTION, "Only one value can be set.");

	char *p = strchr(context->v[0], '=');

	if (p == NULL)
	{
		fprintf(stderr, "Invalid option: %s\n", context->v[0]);
		throw_exception(EXITCODE_INVALID_OPTION, NULL);
	}

	*p = '\0';

	if (strcmp(context->v[0], REGNAME_THRESHOLD) == 0)
	{
		uint16_t threshold_value = atoi(p + 1);

		if (threshold_value > 1024) throw_exception(EXITCODE_INVALID_RANGE, "Value must be smaller than 1024.");

		report_buffer[1] = 0x89; // Update Threshold Opcode
		report_buffer[2] = (threshold_value & 0xFF);
		report_buffer[3] = (threshold_value >> 8);
	}
	else if (strcmp(context->v[0], REGNAME_HYSTERESIS) == 0)
	{
		uint8_t hyst = atoi(p + 1);
		report_buffer[1] = 0x8A; // Update Hystresis Opcode
		report_buffer[2] = (hyst & 0xFF);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Unknown register name.");
	}

	if (TdHidSetReport(context->handle, report_buffer, FEATURE_REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	return 0;
}


static int save(td_context_t* context)
{
	memset(report_buffer, 0, FEATURE_REPORT_SIZE + 1);

	report_buffer[1] = OUTPACKET_SAVE;
	report_buffer[2] = 0x50; // Magic

	if (TdHidSetReport(context->handle, report_buffer, FEATURE_REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	return 0;
}


static int read(td_context_t* context)
{	
	memset(report_buffer, 0, FEATURE_REPORT_SIZE + 1);

	if (TdHidGetReport(context->handle, report_buffer, FEATURE_REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	if (context->c == 0)
	{
		if ( context->format == FORMAT_RAW )
		{
			printf("%d,%d,%d,%d\n",
				report_buffer[5],
				(report_buffer[4] << 8) | report_buffer[3],
				(report_buffer[8] << 8) | report_buffer[7],
				report_buffer[6]);
		}
		else
		{
			printf("%d\n", report_buffer[5]);
		}
	}
	else if (strcmp(context->v[0], REGNAME_INTENSITY) == 0)
	{
		printf("%d\n", (report_buffer[4] << 8) | report_buffer[3]);
	}
	else if (strcmp(context->v[0], REGNAME_THRESHOLD) == 0)
	{
		printf("%d\n", (report_buffer[8] << 8) | report_buffer[7]);
	}
	else if (strcmp(context->v[0], REGNAME_HYSTERESIS) == 0)
	{
		printf("%d\n", report_buffer[6]);
	}
	else if (strcmp(context->v[0], REGNAME_FIRMWARE_VERSION) == 0)
	{
		printf("%d.%d\n", report_buffer[2], report_buffer[1]);
	}
	else if( context->format == FORMAT_RAW )
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Unknown register name.");
	}

	fflush(stdout);

	return 0;
}


static int listen(td_context_t* context)
{
	int result;
	unsigned char in_report_buffer[IN_REPORT_SIZE + 1];
	memset(in_report_buffer, 0, IN_REPORT_SIZE + 1);

	while ((result = TdHidListenReport(context->handle, in_report_buffer, IN_REPORT_SIZE + 1)) == TDHID_ERR_TIMEOUT);

	if (result == TDHID_ERR_IO)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	if (context->format == FORMAT_RAW || context->format == FORMAT_SIMPLE)
	{
		printf("%d\n", in_report_buffer[1]);
		fflush(stdout);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_FORMAT, ERROR_MSG_INVALID_FORMAT);
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
	dt->save = save;

	return dt;
}

td_device_t* (*tdsn450_import)(void) = export_type;
