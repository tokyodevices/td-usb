#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"

static int print_report(int format, uint8_t *buffer)
{
	uint16_t value = ((buffer[6] << 8) | buffer[5]);

	if (format == OPTION_FORMAT_SIMPLE)
	{
		printf("%d\n", value);
	}
	else if (format == OPTION_FORMAT_JSON)
	{
		printf("{ ");		
		printf("\"value\": %d, ", value);
		printf("\"fw_ver_maj\": %d, ", buffer[2]);
		printf("\"fw_ver_min\": %d, ", buffer[1]);
		printf("}\n");
	}

	return 0;
}

static td_device_t *export_type(void)
{
	td_device_t *dt = (td_device_t *)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char *)malloc(7);
	strcpy(dt->product_name, "IWS660");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->input_report_size = 16;
	dt->output_report_size = 16;
	dt->input_report_type = USB_HID_REPORT_TYPE_FEATURE;
	dt->output_report_type = USB_HID_REPORT_TYPE_FEATURE;
	dt->print_report = print_report;
	dt->capability1 = CPBLTY1_CHANGE_SERIAL;

	return dt;
}

td_device_t *(*iws660_import)(void) = export_type;
