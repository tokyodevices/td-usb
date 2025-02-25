/**
* @file tdsn7502.c
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

#define PRODUCT_ID						0x1782

#define REPORT_SIZE						8

#define REGNAME_ZERO_COUNT				"ZERO_COUNT"
#define REGNAME_UNIT_COUNT				"UNIT_COUNT"
#define REGNAME_UNIT_WEIGHT				"UNIT_WEIGHT"
#define REGNAME_LAST_VALUE				"LAST_VALUE"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"

#define ADDR_ZERO_COUNT					0x01
#define ADDR_UNIT_COUNT					0x02
#define ADDR_UNIT_WEIGHT				0x03
#define ADDR_LAST_VALUE					0xF0
#define ADDR_FIRMWARE_VERSION			0xF2


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_ZERO_COUNT) == 0) return ADDR_ZERO_COUNT;
	else if (strcmp(name, REGNAME_UNIT_COUNT) == 0) return ADDR_UNIT_COUNT;
	else if (strcmp(name, REGNAME_UNIT_WEIGHT) == 0) return ADDR_UNIT_WEIGHT;
	else if (strcmp(name, REGNAME_LAST_VALUE) == 0) return ADDR_LAST_VALUE;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return ADDR_FIRMWARE_VERSION;

	fprintf(stderr, "Unknown device register name: %s\n", name);
	throw_exception(EXITCODE_INVALID_OPTION, NULL);

	return 0xFFFF;
}

static int calibrated = 0;
static int32_t offset = 0;
static double scale = 0.0;
static void peform_calibration(td_context_t* context)
{
	if (calibrated == 0)
	{
		offset = tddev2_read_devreg(context, ADDR_ZERO_COUNT);
		int32_t unit_count = tddev2_read_devreg(context, ADDR_UNIT_COUNT);
		uint32_t unit_weight = tddev2_read_devreg(context, ADDR_UNIT_WEIGHT);
		scale = (double)(unit_count - offset) / (double)unit_weight;
		calibrated = 1;
	}
}

static int get(td_context_t* context)
{
	if (context->c == 0)
	{
		context->c = 1;
		context->v[0] = REGNAME_LAST_VALUE;
	}

	uint16_t addr = devreg_name2addr(context->v[0]);
	int32_t value = tddev2_read_devreg(context, addr);

	if (context->format == FORMAT_RAW)
	{
		printf("%d", value);
	}
	else if (context->format == FORMAT_SIMPLE)
	{
		if (addr == ADDR_LAST_VALUE)
		{
			peform_calibration(context);
			int32_t count = (int32_t)value - offset;
			printf("%d", (int)((double)count / scale));
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


static int init(td_context_t* context)
{
	if (context->c != 1)
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Invalid options.");
	}

	int weight = atoi(context->v[0]);

	if ( weight == 0 )
	{
		uint32_t current_count = tddev2_read_devreg(context, ADDR_LAST_VALUE);
		tddev2_write_devreg(context, ADDR_ZERO_COUNT, current_count);
		printf("Done.\n");
	}
	else if (weight < 100)
	{
		printf("The Weight must be grater than 100g. Calibration is not performed.\n");
	}
	else
	{
		uint32_t current_count = tddev2_read_devreg(context, ADDR_LAST_VALUE);		
		tddev2_write_devreg(context, ADDR_UNIT_COUNT, current_count);
		tddev2_write_devreg(context, ADDR_UNIT_WEIGHT, weight);
		printf("Done.\n");
	}

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
	device->init = init;
	device->get = get;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdsn7502_import)(void) = export_type;
