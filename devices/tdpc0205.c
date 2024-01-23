/**
* @file tdpc0205.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID						0x1779

#define REPORT_SIZE						8

#define REGNAME_CONTROL					"CONTROL"
#define REGNAME_TIME					"TIME"
#define REGNAME_DATE					"DATE"
#define REGNAME_FIRMWARE_VERSION		"FIRMWARE_VERSION"

#define REGADDR_CONTROL					0x80
#define REGADDR_TIME					0x87
#define REGADDR_DATE					0x88		
#define REGADDR_FIRMWARE_VERSION		0xF2

static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_CONTROL) == 0) return REGADDR_CONTROL;
	else if (strcmp(name, REGNAME_TIME) == 0) return REGADDR_TIME;
	else if (strcmp(name, REGNAME_DATE) == 0) return REGADDR_DATE;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return REGADDR_FIRMWARE_VERSION;
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

	if (context->c == 0)
	{
		uint32_t value;
		time_t epoc;
		struct tm *ptm;
		char buffer[7];

		time(&epoc); ptm = localtime(&epoc);

		sprintf(buffer, "%02d%02d%02d", ptm->tm_year - 100, ptm->tm_mon + 1, ptm->tm_mday);
		value = strtol(buffer, NULL, 16);
		tddev2_write_devreg(context, REGADDR_DATE, value);

		sprintf(buffer, "%02d%02d%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		value = strtol(buffer, NULL, 16);
		tddev2_write_devreg(context, REGADDR_TIME, value);
	}
	else
	{
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
			uint32_t value = strtol(p + 1, NULL, 16);

			tddev2_write_devreg(context, addr, value);
		}
	}

	return 0;
}


static int get(td_context_t* context)
{
	if (context->c == 0)
	{
		tddev2_write_devreg(context, REGADDR_CONTROL, 1);
		uint32_t date = tddev2_read_devreg(context, REGADDR_DATE);
		uint32_t time = tddev2_read_devreg(context, REGADDR_TIME);

		if (((date >> 16) & 0xFF) == 0xFF)
		{
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "RTC Error");
		}
		else
		{
			printf("20%02X-%02X-%02XT%02X:%02X:%02X",
				(date >> 16) & 0xFF, (date >> 8) & 0xFF, date & 0xFF,
				(time >> 16) & 0xFF, (time >> 8) & 0xFF, time & 0xFF);
		}
	}
	else
	{
		for (int i = 0; i < context->c; i++)
		{
			uint16_t addr = devreg_name2addr(context->v[i]);
			uint32_t value = tddev2_read_devreg(context, addr);
			if (i > 0) printf(",");
			printf("%08X", value);
		}
	}

	printf("\n");
	fflush(stdout);

	return 0;
}

static td_device_t *export_type(void)
{
	td_device_t *device = (td_device_t *)malloc(sizeof(td_device_t));
	memset(device, 0, sizeof(td_device_t));

	device->vendor_id = TOKYODEVICES_VENDOR_ID;
	device->product_id = PRODUCT_ID;
	device->output_report_size = REPORT_SIZE;
	device->input_report_size = REPORT_SIZE;
	device->get = get;
	device->set = set;
	device->destroy = tddev2_destroy_firmware;

	return device;
}

td_device_t *(*tdpc0205_import)(void) = export_type;
