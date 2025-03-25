/**
* @file tdfa1104.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"
#include "../tddevice.h"

#define REPORT_SIZE					8

#define INPACKET_TRIG				0x01

#define REGNAME_CONTROL_FLAG		"CONTROL_FLAG"
#define REGNAME_VRAM				"VRAM"
#define REGNAME_FIRMWARE_VERSION	"FIRMWARE_VERSION"


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_CONTROL_FLAG) == 0) return 0x00;
	else if (strcmp(name, REGNAME_VRAM) == 0) return 0x01;
	else if (strcmp(name, REGNAME_FIRMWARE_VERSION) == 0) return 0xF2;
	else
	{
		fprintf(stderr, "Unknown device register name: %s\n", name);
		throw_exception(EXITCODE_INVALID_OPTION, NULL);
		return 0xFFFF;
	}
}

static uint32_t encode_string(const char* str)
{
	int digit = 0;
	char* pos = str;
	uint32_t data = 0;

	while (TRUE)
	{
		if (*pos == '.')
		{
			data = data | 0x80;
		}
		else
		{
			data = data << 8;
			switch (*pos)
			{
			case '-':
				data = data | 0x40;
				break;
			case '_':
				data = data | 0x08;
				break;
			case '~':
				data = data | 0x01;
				break;
			case 'A':
				data = data | 0x77;
				break;
			case 'C':
				data = data | 0x39;
				break;
			case 'E':
				data = data | 0x79;
				break;
			case 'F':
				data = data | 0x71;
				break;
			case 'G':
				data = data | 0x3D;
				break;
			case 'H':
				data = data | 0x76;
				break;
			case 'I':
				data = data | 0x30;
				break;
			case 'J':
				data = data | 0x0E;
				break;
			case 'L':
				data = data | 0x38;
				break;
			case 'P':
				data = data | 0x73;
				break;
			case 'U':
				data = data | 0x3E;
				break;
			case 'Y':
				data = data | 0x72;
				break;
			case 'b':
				data = data | 0x7C;
				break;
			case 'c':
				data = data | 0x58;
				break;
			case 'd':
				data = data | 0x5E;
				break;
			case 'h':
				data = data | 0x74;
				break;
			case 'n':
				data = data | 0x54;
				break;
			case 'o':
				data = data | 0x5C;
				break;
			case 'r':
				data = data | 0x50;
				break;
			case 't':
				data = data | 0x78;
				break;
			case 'u':
				data = data | 0x1C;
				break;
			case 'D':
			case 'O':
			case '0':
				data = data | 0x3F;
				break;
			case '1':
				data = data | 0x06;
				break;
			case '2':
				data = data | 0x5B;
				break;
			case '3':
				data = data | 0x4F;
				break;
			case '4':
				data = data | 0x66;
				break;
			case '5':
				data = data | 0x6D;
				break;
			case '6':
				data = data | 0x7D;
				break;
			case '7':
				data = data | 0x07;
				break;
			case 'B':
			case '8':
				data = data | 0x7F;
				break;
			case '9':
				data = data | 0x6F;
				break;
			}
			digit++;
		}

		if (digit == 4) break;

		pos++;
		if (*pos == '\0') break;
	}

	return data;
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
			uint32_t value = encode_string(context->v[i]);
			tddev2_write_devreg(context, 0x01, value);
		}
		else
		{
			*p = '\0';
			uint16_t addr = devreg_name2addr(context->v[i]);
			uint32_t value = encode_string(p + 1);
			tddev2_write_devreg(context, addr, value);
		}
	}

	return 0;
}


static int get(td_context_t* context)
{
	if (context->c == 0) // Default behavier of read operation
	{
		context->c = 1;
		context->v[0] = REGNAME_VRAM;
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


static td_device_t* export_type(void)
{
	td_device_t* device = (td_device_t*)malloc(sizeof(td_device_t));
	memset(device, 0, sizeof(td_device_t));

	device->vendor_id = TOKYODEVICES_VENDOR_ID;
	device->product_id = 6019;
	device->output_report_size = REPORT_SIZE;
	device->input_report_size = REPORT_SIZE;
	device->get = get;
	device->set = set;
	device->destroy = tddev2_destroy_firmware;
	device->save = tddev2_save_to_flash;

	return device;
}

td_device_t* (*tdfa1104_import)(void) = export_type;
