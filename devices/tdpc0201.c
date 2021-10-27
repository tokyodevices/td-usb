/**
* @file tdpc0201.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"


#define REPORT_SIZE						8

static uint8_t report_buffer[REPORT_SIZE+1];


#define REGNAME_CONTROL_FLAG			"CONTROL_FLAG"
#define REGNAME_WATCHING_TIME			"WATCHING_TIME"
#define REGNAME_WARNING_TIME			"WARNING_TIME"
#define REGNAME_ACTIVATING_TIME			"ACTIVATING_TIME"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"

#define VERB_FORCE_ACTIVATE				"FORCE_ACTIVATE"
#define VERB_WATCHDOG_CLEAR				"WATCHDOG_CLEAR"

#define CONTROL_FLAG_BUZZER_ENABLE		1
#define CONTROL_FLAG_LED_ENABLE			2
#define CONTROL_FLAG_TICK_ENABLE		4

#define INPACKET_TICK					0x01

#define OUTPACKET_WDGCLR				0x88
#define OUTPACKET_ACT					0x89

#define DEVREG_FIRMWARE_VERSION			0xF2


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_CONTROL_FLAG) == 0) return 0x00;
	else if (strcmp(name, REGNAME_WATCHING_TIME) == 0) return 0x01;
	else if (strcmp(name, REGNAME_WARNING_TIME) == 0) return 0x02;
	else if (strcmp(name, REGNAME_ACTIVATING_TIME) == 0) return 0x03;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return 0xF2;
	else
	{
		fprintf(stderr, "Unknown device register name: %s\n", name);
		throw_exception(EXITCODE_INVALID_OPTION, NULL);
		return 0xFFFF;
	}
}

static void wdg_clear(td_context_t* context)
{
	memset(report_buffer, 0, REPORT_SIZE + 1);

	report_buffer[0] = 0x00;        // Dummy report Id
	report_buffer[1] = OUTPACKET_WDGCLR;        // OUTPACKET_WDGCLR

	DEBUG_PRINT(("Sending OUTPACKET_WDGCLR command.\n"));
	if (TdHidSetReport(context->handle, report_buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_OUTPUT))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	DEBUG_PRINT(("Listening INPACKET_ACK reply.\n"));
	if (TdHidListenReport(context->handle, report_buffer, REPORT_SIZE + 1) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	if (report_buffer[1] != INPACKET_ACK)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid reply.");
}


static void force_activate(td_context_t* context)
{	
	memset(report_buffer, 0, REPORT_SIZE + 1);

	report_buffer[0] = 0x00;        // Dummy report Id
	report_buffer[1] = OUTPACKET_ACT;        // OUTPACKET_ACT

	DEBUG_PRINT(("Sending OUTPACKET_ACT command.\n"));
	if (TdHidSetReport(context->handle, report_buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_OUTPUT))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
}


static int set(td_context_t* context)
{
	char* p;

	if (context->c == 0)
	{
		wdg_clear(context);	
	}
	else
	{
		for (int i = 0; i < context->c; i++)
		{
			if (strcmp(context->v[i], VERB_FORCE_ACTIVATE) == 0)
			{
				force_activate(context);
			}
			else if (strcmp(context->v[i], VERB_WATCHDOG_CLEAR) == 0)
			{
				wdg_clear(context);
			}
			else
			{
				p = strchr(context->v[i], '=');

				if (p == NULL)
				{
					fprintf(stderr, "Invalid option: %s\n", context->v[i]);
					throw_exception(EXITCODE_INVALID_OPTION, NULL);
				}

				*p = '\0';

				uint16_t addr = devreg_name2addr(context->v[i]);
				uint32_t value = atoi(p + 1);

				tddev2_write_devreg(context, addr, value);
			}
		}
	}

	return 0;
}


static int get(td_context_t* context)
{
	if (context->c == 0) // Default behavier of read operation
	{
		context->c = 1;
		context->v[0] = REGNAME_FIRMWARE_VERSION;
	}

	for (int i = 0; i < context->c; i++)
	{
		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = tddev2_read_devreg(context, addr);

		if (context->format == FORMAT_RAW || context->format == FORMAT_SIMPLE)
		{
			if (i > 0) printf(",");
			printf("%d", value);
		}
		else
		{
			throw_exception(EXITCODE_INVALID_FORMAT, "Unknown format");
		}
	}

	printf("\n");
	fflush(stdout);

	return 0;
}


static int listen(td_context_t* context)
{	
	memset(report_buffer, 0, REPORT_SIZE + 1);

	if (TdHidListenReport(context->handle, report_buffer, REPORT_SIZE + 1) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	printf("%d,%d\n", report_buffer[1], report_buffer[2]);
	fflush(stdout);

	return 0;
}



static int init(td_context_t* context)
{
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_CONTROL_FLAG), 3);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_WATCHING_TIME), 180);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_WARNING_TIME), 30);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_ACTIVATING_TIME), 30);
	printf("Device has been Initialized.\n");
	return 0;
}

static td_device_t* export_type(void)
{
	td_device_t* device = (td_device_t*)malloc(sizeof(td_device_t));
	memset(device, 0, sizeof(td_device_t));

	device->vendor_id = TOKYODEVICES_VENDOR_ID;
	device->product_id = 6003;
	device->output_report_size = REPORT_SIZE;
	device->input_report_size = REPORT_SIZE;
	device->get = get;
	device->set = set;
	device->listen = listen;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;
	device->init = init;

	return device;
}

td_device_t* (*tdpc0201_import)(void) = export_type;
