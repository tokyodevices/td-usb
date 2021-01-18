#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"

static int print_report(int format, uint8_t *buffer)
{
	unsigned short average = ((buffer[4] << 8) | buffer[3]);
	uint16_t cal = ((buffer[8] << 8) | buffer[7]);

	if (format == OPTION_FORMAT_SIMPLE)
	{
		printf("%d\n", average);
	}
	else if (format == OPTION_FORMAT_JSON)
	{
		printf("{ ");
		printf("\"fw_ver_maj\": %d, ", buffer[2]);
		printf("\"fw_ver_min\": %d, ", buffer[1]);
		printf("\"cal\": %d, ", cal);
		printf("\"val\": %d ", average);
		printf("}\n");
	}

	return 0;
}

static int prepare_report(int format, const char *report_string, uint8_t *buffer)
{
	uint16_t cal_val1, cal_val2;

	int v = atoi(report_string);

	if (v < 0 || v >= 1024)
	{
		fprintf(stderr, "Range error. Must be 0-1023.\n");
		return 1;
	}

	cal_val1 = (uint16_t)v;
	cal_val2 = 0xFFFF; // unused 

	buffer[0] = 0x00;	
	buffer[1] = 0x89;
	buffer[2] = (uint8_t)(cal_val1 & 0xFF);
	buffer[3] = (uint8_t)(cal_val1 >> 8);
	buffer[4] = (uint8_t)(cal_val2 & 0xFF);
	buffer[5] = (uint8_t)(cal_val2 >> 8);

	return 0;
}

static td_device_t *export_type(void)
{
	td_device_t *dt = (td_device_t *)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char *)malloc(7);
	strcpy(dt->product_name, "IWS450");
	
	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->report_size = 16;
	dt->input_report_type = USB_HID_REPORT_TYPE_FEATURE;
	dt->output_report_type = USB_HID_REPORT_TYPE_FEATURE;
	dt->print_report = print_report;
	dt->prepare_report = prepare_report;
	dt->capability1 = CPBLTY1_CHANGE_SERIAL;

	return dt;
}

td_device_t *(*iws450_import)(void) = export_type;
