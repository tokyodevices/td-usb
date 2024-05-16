/**
* @file tdsn0700.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define PRODUCT_ID					0x177A

#define REPORT_SIZE					8

#define REGNAME_CONTROL_FLAG		"CONTROL_FLAG"
#define REGNAME_OFFSET				"OFFSET"			// Offset (ADC count)
#define REGNAME_CAL_INTENSITY		"CAL_INTENSITY"		// ADC count on reference intensity
#define REGNAME_CAL_REFERENCE		"CAL_REFERENCE"		// ADC count of reference intensity
#define REGNAME_DECADE_VOLTAGE		"DECADE_VOLTAGE"	// Voltage per decade (mV)
#define REGNAME_SENSITIVITY			"SENSITIVITY"		// Sensitivity (nA)
#define REGNAME_INTENSITY			"INTENSITY"
#define REGNAME_FIRMWARE_VERSION	"FIRMWARE_VERSION"

#define REGADDR_CONTROL_FLAG		0x00
#define REGADDR_OFFSET				0x01
#define REGADDR_CAL_INTENSITY		0x02
#define REGADDR_CAL_REFERENCE		0x03
#define REGADDR_DECADE_VOLTAGE		0x04
#define REGADDR_SENSITIVITY			0x05
#define REGADDR_INTENSITY			0xF0
#define REGADDR_FIRMWARE_VERSION	0xF2

#define ADC_MAX_VALUE				4095
#define ADC_VREF					1.238

static uint8_t calibration_available = 0;
static unsigned int offset;
static double gain;
static double voltage_per_decade;
static double nanoamp_per_mWcm2;

static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_CONTROL_FLAG) == 0) return REGADDR_CONTROL_FLAG;
	else if (strcmp(name, REGNAME_OFFSET) == 0) return REGADDR_OFFSET;
	else if (strcmp(name, REGNAME_CAL_INTENSITY) == 0) return REGADDR_CAL_INTENSITY;
	else if (strcmp(name, REGNAME_CAL_REFERENCE) == 0) return REGADDR_CAL_REFERENCE;
	else if (strcmp(name, REGNAME_DECADE_VOLTAGE) == 0) return REGADDR_DECADE_VOLTAGE;
	else if (strcmp(name, REGNAME_SENSITIVITY) == 0) return REGADDR_SENSITIVITY;
	else if (strcmp(name, REGNAME_INTENSITY) == 0) return REGADDR_INTENSITY;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return REGADDR_FIRMWARE_VERSION;
	else
	{
		fprintf(stderr, "Unknown device register name: %s\n", name);
		throw_exception(EXITCODE_INVALID_OPTION, NULL);
		return 0xFFFF;
	}
}

static void load_calibration_value(td_context_t* context)
{
	if (calibration_available != 0) return;

	uint32_t v;

	// Offset
	offset = tddev2_read_devreg(context, REGADDR_OFFSET);

	// Gain
	v = tddev2_read_devreg(context, REGADDR_CAL_INTENSITY);
	double measured_intensity = v;
	v = tddev2_read_devreg(context, REGADDR_CAL_REFERENCE);
	double ref_intensity = v;
	gain = ref_intensity / measured_intensity;
		
	// Decade voltage in mV
	v = tddev2_read_devreg(context, REGADDR_DECADE_VOLTAGE);
	voltage_per_decade = (double)v / 1000.0;
	
	// Sensitivity
	v = tddev2_read_devreg(context, REGADDR_SENSITIVITY);
	nanoamp_per_mWcm2 = (double)v;

	calibration_available = 1;

	return;
}


static int set(td_context_t* context)
{
	char* p;

	if (context->c == 0) throw_exception(EXITCODE_INVALID_OPTION, "Device Register is not specified.");

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
		context->v[0] = REGNAME_INTENSITY;
	}

	for (int i = 0; i < context->c; i++)
	{
		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = tddev2_read_devreg(context, addr);

		if (i > 0) printf(",");

		if (strcmp(context->v[i], REGNAME_INTENSITY) == 0)
		{
			if (context->format == FORMAT_SIMPLE)
			{
				load_calibration_value(context);

				if (value <= offset)
				{
					printf("-1");
				}
				else if (value == ADC_MAX_VALUE)
				{
					printf("-2");
				}
				else
				{
					double max4206_voltage = (ADC_VREF * (double)value * gain / (double)ADC_MAX_VALUE);
					double intensity_nano_amp = pow(10.0, max4206_voltage / voltage_per_decade) * 10.0;
					int intensity_uW_per_cm2 = (int)(intensity_nano_amp * 1000.0 / nanoamp_per_mWcm2);
					printf("%d", intensity_uW_per_cm2);
				}
			}
			else if (context->format == FORMAT_RAW)
			{
				printf("%d", value);
			}
			else
			{
				throw_exception(EXITCODE_INVALID_FORMAT, "Unknown format");
			}
		}
		else
		{
			printf("%d", value);
		}
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
		if ((TdHidListenReport(context->handle, buffer, REPORT_SIZE + 1)) != TDHID_SUCCESS)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
		if (buffer[1] == INPACKET_DUMP) break;
	}

	uint16_t value = (buffer[3] << 8) | buffer[2];

	if (context->format == FORMAT_RAW)
	{
		printf("%d", value);
	}
	else
	{
		if (value <= offset)
		{
			printf("-1");
		}
		else if (value == ADC_MAX_VALUE)
		{
			printf("-2");
		}
		else
		{
			double max4206_voltage = (ADC_VREF * (double)value * gain / (double)ADC_MAX_VALUE);
			double intensity_nano_amp = pow(10.0, max4206_voltage / voltage_per_decade) * 10.0;
			int intensity_uW_per_cm2 = (int)(intensity_nano_amp * 1000.0 / nanoamp_per_mWcm2);
			printf("%d", intensity_uW_per_cm2);
		}
	}

	printf("\n");
	fflush(stdout);

	return 0;
}

static int init(td_context_t* context)
{
	if (context->c == 1)
	{
		// ゲイン校正値を設定する (第3オプションは真の照度として与えられた値(単位: uW/cm2))
		int uwcm2 = atoi(context->v[0]);

		if (uwcm2 < 1000 || uwcm2 > 50000)
			throw_exception(EXITCODE_INVALID_OPTION, "Refference intensity is out of range. Should be 1,000 to 50,000 uW/cm2.");

		load_calibration_value(context);

		// 真値として与えられた照度を単位変換
		double pd_current = (uwcm2 / 1000.0) * nanoamp_per_mWcm2;
		double max4206_vout = log(pd_current / 10.0) * voltage_per_decade;
		uint32_t ref_intensity = (uint32_t)(((double)ADC_MAX_VALUE * max4206_vout) / ADC_VREF);

		// 現在の照度を取得
		uint32_t current_intensity = tddev2_read_devreg(context, REGADDR_INTENSITY);

		// デバイスレジスタに保存
		tddev2_write_devreg(context, REGADDR_CAL_INTENSITY, current_intensity);
		tddev2_write_devreg(context, REGADDR_CAL_REFERENCE, ref_intensity);
		tddev2_save_to_flash(context);

		printf("CAL_INTENSITY and CAL_REFERENCE are updated (%04Xh / %04Xh).", current_intensity, ref_intensity);
	}
	else
	{
		throw_exception(EXITCODE_INVALID_OPTION, "Invalid options.");
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
	device->get = get;
	device->set = set;
	device->init = init;
	device->listen = listen;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdsn0700_import)(void) = export_type;
