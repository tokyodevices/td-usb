#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "../td-usb.h"
#include "../tdhid.h"

#define REPORT_SIZE		16


static int read(td_context_t* context)
{
	uint8_t buffer[REPORT_SIZE + 1];
	uint16_t value, cal;
	double L;

	if (TdHidGetReport(context->handle, buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	value = ((buffer[6] << 8) | buffer[5]);
	cal = ((buffer[10] << 8) | buffer[9]);
	L = pow(10.0, (double)(value) / (0.22 * (double)cal));

	if (context->format == FORMAT_SIMPLE)
	{
		printf("%.1f\n", L);
	}
	else if (context->format == FORMAT_JSON)
	{
		printf("{ ");		
		printf("\"value\": %.1f, ", L);
		printf("\"fw_ver_maj\": %d, ", buffer[2]);
		printf("\"fw_ver_min\": %d, ", buffer[1]);
		printf("}\n");
	}

	return 0;
}

static td_device_t *export_type(void)
{
	td_device_t* dt = (td_device_t *)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char *)malloc(7);
	strcpy(dt->product_name, "IWS660");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->output_report_size = REPORT_SIZE;
	dt->read = read;
	dt->capability1 = CPBLTY1_CHANGE_SERIAL;

	return dt;
}

td_device_t *(*iws660_import)(void) = export_type;
