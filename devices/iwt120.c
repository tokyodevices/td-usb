// iwt120.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"

static int print_report(int format, uint8_t *buffer)
{
	if (format == OPTION_FORMAT_SIMPLE)
	{
		printf("%d\n", buffer[2]);
	}
	else if (format == OPTION_FORMAT_JSON)
	{
		printf("{ ");
		printf("\"fw_ver\": %d, ", buffer[1]);
		printf("\"val\": %d, ", buffer[2]);		
		printf("}\n");
	}

	return 0;
}


static int prepare_report(int format, const char *report_string, uint8_t *buffer)
{
	int v = atoi(report_string);

	if (v < 0 || v >= 256)
	{
		fprintf(stderr, "Value range error. It must be in 0-255.\n");
		return 1;
	}

	buffer[0] = 0x00;
	buffer[1] = 0x31; // Set mode command
	buffer[2] = (uint8_t)v;

	return 0;
}

static td_device_t *export_type(void)
{
	td_device_t *dt = (td_device_t *)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char *)malloc(7);
	strcpy(dt->product_name, "IWT120");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->input_report_size = 16;
	dt->output_report_size = 16;
	dt->input_report_type = USB_HID_REPORT_TYPE_FEATURE;
	dt->output_report_type = USB_HID_REPORT_TYPE_FEATURE;
	dt->print_report = print_report;
	dt->prepare_report = prepare_report;
	dt->capability1 = CPBLTY1_CHANGE_SERIAL;

	return dt;
}

td_device_t *(*iwt120_import)(void) = export_type;
