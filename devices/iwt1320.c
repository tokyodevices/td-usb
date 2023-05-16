/**
* @file iwt1320.c
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

static uint8_t buffer[REPORT_SIZE + 1];

#define REGNAME_BUZZER_CONTROL		"BUZZER_CONTROL"
#define REGNAME_LED_CONTROL			"LED_CONTROL"

static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_BUZZER_CONTROL) == 0) return 0x83;
	else if (strcmp(name, REGNAME_LED_CONTROL) == 0) return 0x84;
	else
	{
		fprintf(stderr, "Unknown device register name: %s\n", name);
		throw_exception(EXITCODE_INVALID_OPTION, NULL);
		return 0xFFFF;
	}
}

static int set(td_context_t* context)
{
	char* p;

	if (context->c == 0) throw_exception(EXITCODE_INVALID_OPTION, "No option is specified.");

	for (int i = 0; i < context->c; i++)
	{
		p = strchr(context->v[i], '=');

		if (p == NULL)
		{
			fprintf(stderr, "Invalid option: %s\n", context->v[i]);
			throw_exception(EXITCODE_INVALID_OPTION, NULL);
		}

		*p = '\0';

		memset(buffer, 0, REPORT_SIZE + 1);
		buffer[0] = 0x00; // Dummy report id
		buffer[1] = (uint8_t)devreg_name2addr(context->v[i]); // Address of a device register
		buffer[2] = (uint8_t)atoi(p + 1); // value

		if (TdHidSetReport(context->handle, buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
	}

	return 0;
}

static int get(td_context_t* context)
{
	uint8_t buffer[REPORT_SIZE + 1];
	uint16_t value, cal;
	double L;

	if (TdHidGetReport(context->handle, buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_FEATURE))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	value = ((buffer[3] << 8) | buffer[4]);

	if (context->format == FORMAT_SIMPLE)
	{
		printf("%d\n", value);
	}
	else if (context->format == FORMAT_JSON)
	{
		printf("{ ");
		printf("\"value\": %d, ", value);
		printf("\"fw_ver_min\": %d, ", buffer[2]);
		printf("\"fw_ver_maj\": %d, ", buffer[1]);
		printf("}\n");
	}

	fflush(stdout);

	return 0;
}

static td_device_t* export_type(void)
{
	td_device_t* dt = (td_device_t*)malloc(sizeof(td_device_t));
	memset(dt, 0, sizeof(td_device_t));

	dt->product_name = (char*)malloc(7);
	strcpy(dt->product_name, "IWT1320");

	dt->vendor_id = 0x16c0;
	dt->product_id = 0x05df;
	dt->output_report_size = REPORT_SIZE;
	dt->set = set;
	dt->get = get;
	dt->init = tddev1_init_operation;

	return dt;
}

td_device_t* (*iwt1320_import)(void) = export_type;
