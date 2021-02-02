// iwt120.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"


#define REPORT_SIZE		16


static int write(td_context_t* context)
{
	uint8_t report_buffer[REPORT_SIZE + 1];
	memset(report_buffer, 0, REPORT_SIZE + 1);

	if (context->c != 1)
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Just one property can be set.");
	}

	report_buffer[0] = 0x00; // Dummy report id
	report_buffer[1] = 0x31; // Set mode command
	report_buffer[2] = (uint8_t)atoi(context->v[0]); // Register value

	if (TdHidSetReport(context->handle, report_buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	return 0;
}


static td_device_t *export_type(void)
{
	td_device_t* dt = (td_device_t *)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char *)malloc(7);
	strcpy(dt->product_name, "IWT120");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->output_report_size = REPORT_SIZE;	
	dt->capability1 = CPBLTY1_CHANGE_SERIAL;
	dt->write = write;

	return dt;
}

td_device_t *(*iwt120_import)(void) = export_type;
