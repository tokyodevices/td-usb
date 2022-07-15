/**
* @file tdfa30604.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define TDFA30604_PRODUCT_ID		0x1774

#define REPORT_SIZE					8

#define INPACKET_TRIG				0x01

#define REGNAME_CONTROL_FLAG		"CONTROL_FLAG"
#define REGNAME_ANTI_CHAT_LEN		"ANTI_CHAT_LEN"
#define REGNAME_TRIGGER_RISING		"TRIGGER_RISING"
#define REGNAME_TRIGGER_FALLING		"TRIGGER_FALLING"
#define REGNAME_GPIO_STATUS			"GPIO_STATUS"
#define REGNAME_FIRMWARE_VERSION	"FIRMWARE_VERSION"


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_CONTROL_FLAG) == 0) return 0x00;
	else if (strcmp(name, REGNAME_ANTI_CHAT_LEN) == 0) return 0x01;
	else if (strcmp(name, REGNAME_TRIGGER_RISING) == 0) return 0x04;
	else if (strcmp(name, REGNAME_TRIGGER_FALLING) == 0) return 0x05;
	else if (strcmp(name, REGNAME_GPIO_STATUS) == 0) return 0x87;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return 0xF2;
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

		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = atoi(p + 1);

		tddev2_write_devreg(context, addr, value);
	}

	return 0;
}


static int get(td_context_t* context)
{
	if (context->c == 0) // Default behavier of read operation
	{
		context->c = 1;
		context->v[0] = REGNAME_GPIO_STATUS;
	}

	for(int i=0 ; i < context->c ; i++)
	{
		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = tddev2_read_devreg(context, addr);

		if (context->format == FORMAT_RAW || context->format == FORMAT_SIMPLE)
		{
			if( i > 0 ) printf(",");
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
	unsigned char report_buffer[REPORT_SIZE + 1];
	memset(report_buffer, 0, REPORT_SIZE + 1);

	if (TdHidListenReport(context->handle, report_buffer, REPORT_SIZE + 1) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	if (report_buffer[1] != INPACKET_TRIG )
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid reply.");


	if (context->format == FORMAT_RAW || context->format == FORMAT_SIMPLE)
	{
		printf("%d,%d\n", report_buffer[2], report_buffer[4]);
		fflush(stdout);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_FORMAT, "Unknown format");
	}

	return 0;
}


static int init(td_context_t* context)
{
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_ANTI_CHAT_LEN), 20);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_TRIGGER_RISING), 0);
	tddev2_write_devreg(context, devreg_name2addr(REGNAME_TRIGGER_FALLING), 0);
	
	printf("Device has been Initialized.\n");

	return 0;
}


static td_device_t *export_type(void)
{
	td_device_t *device = (td_device_t *)malloc(sizeof(td_device_t));
	memset(device, 0, sizeof(td_device_t));

	device->vendor_id = TOKYODEVICES_VENDOR_ID;
	device->product_id = TDFA30604_PRODUCT_ID;
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

td_device_t *(*tdfa30604_import)(void) = export_type;
