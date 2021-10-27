/**
* @file iws73X.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
* @date 2020-2-12
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"


#define REPORT_SIZE		16


static int write(td_context_t* context)
{
	uint8_t report_buffer[REPORT_SIZE + 1];
	memset(report_buffer, 0, REPORT_SIZE + 1);

	if (context->c != 1)
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Only one value can be set.");
	}

	if (strncmp(context->v[0], "LED=", 4) == 0)
	{
		report_buffer[0] = 0x00; // Dummy report id
		report_buffer[1] = 0x81; // LED register address
		report_buffer[2] = (context->v[0][4] == '1') ? 1 : 0;
	}
	else if (strcmp(context->v[0], "CAL") == 0)
	{
		report_buffer[0] = 0x00; // Dummy report id
		report_buffer[1] = 0x83; // CAL register address
		printf("started calibration.\n");
	}
	else
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Unknown register name.");
	}

	if (TdHidSetReport(context->handle, report_buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	return 0;
}


static int read(td_context_t* context)
{
	time_t epoc;
	uint8_t buffer[REPORT_SIZE + 1];
	memset(buffer, 0, REPORT_SIZE + 1);
	if (TdHidGetReport(context->handle, buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	unsigned short gas_ppm = ((unsigned char)buffer[1] << 8) | (unsigned char)buffer[2];
	unsigned short status = ((unsigned char)buffer[3] << 8) | (unsigned char)buffer[4];
	unsigned short temperature = ((unsigned char)buffer[5] << 8) | (unsigned char)buffer[6];
	unsigned short humidity = ((unsigned char)buffer[7] << 8) | (unsigned char)buffer[8];

	double T = -40.1 + (0.01 * (double)temperature);
	double RHliner = -2.0468 + (0.0367 * (double)humidity) + (-1.5955E-6 * (double)(humidity * humidity));
	double RHtrue = ((double)T - 25.0) * (0.01 + 0.00008 * (double)humidity) + RHliner;

	time(&epoc);

	if (context->c == 0)
	{
		printf("%llu,%u,%u,%f,%f\n", epoc, status, gas_ppm, T, RHtrue);
		fflush(stdout);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Unknown option.");
	}

	return 0;
}


static td_device_t* export_type(void)
{
	td_device_t* dt = (td_device_t*)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char*)malloc(7);
	strcpy(dt->product_name, "IWS730");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->output_report_size = REPORT_SIZE;
	dt->init = tddev1_init_operation;
	dt->set = write;
	dt->get = read;

	return dt;
}

td_device_t* (*iws73x_import)(void) = export_type;
