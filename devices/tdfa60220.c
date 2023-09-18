/**
* @file tdfa60220.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID						0x1775

#define REPORT_SIZE						64

#define REGNAME_CONTROL_FLAG			"CONTROL_FLAG"
#define REGNAME_CALIBRATION				"CALIBRATION"
#define REGNAME_ADC_VALUE				"ADC_VALUE"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"

#define ADDR_CONTROL_FLAG				0x00
#define ADDR_CALIBRATION				0x01
#define ADDR_ADC_VALUE					0xF0
#define ADDR_FIRMWARE_VERSION			0xF2


static uint8_t calibration_available = 0;
static float ADCVALUE_25MA = 0.0;


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_CONTROL_FLAG) == 0) return ADDR_CONTROL_FLAG;
	else if (strcmp(name, REGNAME_CALIBRATION) == 0) return ADDR_CALIBRATION;
	else if (strcmp(name, REGNAME_ADC_VALUE) == 0) return ADDR_ADC_VALUE;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return ADDR_FIRMWARE_VERSION;
	
	fprintf(stderr, "Unknown device register name: %s\n", name);
	throw_exception(EXITCODE_INVALID_OPTION, NULL);

	return 0xFFFF;
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


static void load_calibration_value(td_context_t* context)
{
	if (calibration_available != 0) return;

	ADCVALUE_25MA = (float)tddev2_read_devreg(context, ADDR_CALIBRATION);

	calibration_available = 1;

	return;
}


static int get(td_context_t* context)
{
	if (context->c == 0)
	{
		context->c = 1;
		context->v[0] = REGNAME_ADC_VALUE;
	}
	
	uint16_t addr = devreg_name2addr(context->v[0]);
	uint32_t value = tddev2_read_devreg(context, addr);

	if (context->format == FORMAT_RAW)
	{
		printf("%d", value);
	}
	else if (context->format == FORMAT_SIMPLE)
	{
		if (addr == ADDR_ADC_VALUE)
		{
			load_calibration_value(context);
			printf("%f", 25.0 * (float)value / ADCVALUE_25MA);
		}
		else if (addr == ADDR_FIRMWARE_VERSION)
		{
			printf("%d.%d", (value >> 8), (value & 0xFF));
		}
		else
		{
			printf("%d", value);
		}
	}
	else
	{
		throw_exception(EXITCODE_INVALID_FORMAT, ERROR_MSG_INVALID_FORMAT);
	}

	printf("\n");
	fflush(stdout);

	return 0;
}


static int listen(td_context_t* context)
{
	uint8_t buffer[REPORT_SIZE + 1];

	load_calibration_value(context);

	while (1)
	{
		if ((TdHidListenReport(context->handle, buffer, REPORT_SIZE + 1)) != 0)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
		if (buffer[1] == INPACKET_DUMP) break;
	}		

	for (int i = 0; i < buffer[2]; i++)
	{
		uint16_t value = (buffer[(i * 2) + 4] << 8) | buffer[(i * 2) + 3];

		if (context->format == FORMAT_RAW)
		{
			printf("%d\n", value);
		}
		else
		{
			printf("%f\n", 25.0 * (float)value / ADCVALUE_25MA);
		}
	}
	
	fflush(stdout);

	return 0;
}


static td_device_t* export_type(void)
{
	td_device_t* device = (td_device_t*)malloc(sizeof(td_device_t));
	memset(device, 0, sizeof(td_device_t));

	device->vendor_id = TOKYODEVICES_VENDOR_ID;
	device->product_id = PRODUCT_ID;
	device->output_report_size = REPORT_SIZE;
	device->input_report_size = REPORT_SIZE;
	device->get = get;
	device->set = set;
	device->listen = listen;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdfa60220_import)(void) = export_type;
