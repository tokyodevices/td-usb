#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define REPORT_SIZE		16

static uint8_t buffer[REPORT_SIZE + 1];

static int write(td_context_t* context)
{
	uint8_t port_state;

	if (context->c != 1)
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Only one value can be set.");
	}

	port_state = atoi(context->v[0]);

	memset(buffer, 0, REPORT_SIZE + 1);

	buffer[0] = 0x00; // Dummy report id
	buffer[1] = 0x81; // Set port state command
	buffer[2] = port_state; // Port state

	if (TdHidSetReport(context->handle, buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	return 0;
}


static int read(td_context_t* context)
{
	if (TdHidGetReport(context->handle, buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	if (context->format == FORMAT_SIMPLE)
	{
		printf("%d\n", buffer[3]);
	}
	else if (context->format == FORMAT_JSON)
	{
		printf("{ ");		
		printf("\"fw_ver_maj\": %d, ", buffer[2]);
		printf("\"fw_ver_min\": %d, ", buffer[1]);
		printf("\"value\": %d", buffer[3]);
		printf(" }\n");
	}

	return 0;
}


static td_device_t* iwt303_export_type(void)
{
	td_device_t* dt = (td_device_t*)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char*)malloc(7);
	strcpy(dt->product_name, "IWT303");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->output_report_size = REPORT_SIZE;
	dt->get = read;
	dt->set = write;
	dt->init = tddev1_init_operation;

	return dt;
}


static td_device_t* iwt313_export_type(void)
{
	td_device_t* dt = (td_device_t*)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char*)malloc(7);
	strcpy(dt->product_name, "IWT313");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->output_report_size = REPORT_SIZE;
	dt->get = read;
	dt->set = write;
	dt->init = tddev1_init_operation;

	return dt;
}

td_device_t* (*iwt303_import)(void) = iwt303_export_type;
td_device_t* (*iwt313_import)(void) = iwt313_export_type;
