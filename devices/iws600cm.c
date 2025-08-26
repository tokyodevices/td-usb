/**
* @file iws600cm.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define REPORT_SIZE		16


static int read(td_context_t* context)
{
	uint8_t buffer[REPORT_SIZE + 1];
	uint8_t value;

	if (TdHidGetReport(context->handle, buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	value = buffer[1];

	printf("%d\n", value);

	fflush(stdout);

	return 0;
}

static td_device_t* export_type(void)
{
	td_device_t* dt = (td_device_t*)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char*)malloc(10);
	strcpy(dt->product_name, "IWS600-CM");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->output_report_size = REPORT_SIZE;
	dt->get = read;
	dt->init = tddev1_init_operation;

	return dt;
}

td_device_t* (*iws600cm_import)(void) = export_type;
