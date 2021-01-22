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
		printf("%d\n", (unsigned char)buffer[3]);
	}
	else if (format == OPTION_FORMAT_JSON)
	{
		printf("{ ");
		printf("\"firmware_version\": %d.%d, ", (unsigned char)buffer[1], (unsigned char)buffer[2]);
		printf("\"last_status\": %d, ", (unsigned char)buffer[3]);
		printf("}\n");
	}
	return 0;
}

static td_device_t *export_type(void)
{
	td_device_t *dt = (td_device_t *)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->vendor_id = TOKYODEVICES_VENDOR_ID;
	dt->product_id = 6000;
	dt->input_report_size = 8;
	dt->output_report_size = 8;
	dt->print_report = print_report;
	dt->input_report_type = USB_HID_REPORT_TYPE_INPUT;
	dt->output_report_type = USB_HID_REPORT_TYPE_OUTPUT;
	dt->capability1 = CPBLTY1_DFU_AFTER_ERASE | CPBLTY1_LISTENABLE;
	
	return dt;
}

td_device_t *(*tdfa30608_import)(void) = export_type;
