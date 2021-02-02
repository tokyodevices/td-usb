#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../td-usb.h"
#include "../tdhid.h"

#define REPORT_SIZE					8

#define REGNAME_ANTI_CHAT_LEN		"ANTI_CHAT_LEN"
#define REGNAME_TRIGGER_RISING		"TRIGGER_RISING"
#define REGNAME_TRIGGER_FALLING		"TRIGGER_FALLING"
#define REGNAME_GPIO_STATUS			"GPIO_STATUS"
#define REGNAME_FIRMWARE_VERSION	"FIRMWARE_VERSION"


static uint16_t devreg_name2addr(char* name)
{
	if (strcmp(name, REGNAME_ANTI_CHAT_LEN) == 0) return 0x01;
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


static void write_devreg(td_context_t *context, uint16_t addr, uint32_t value)
{
	unsigned char report_buffer[REPORT_SIZE + 1];

	memset(report_buffer, 0, REPORT_SIZE + 1);	

	report_buffer[0] = 0x00;        // Dummy report Id
	report_buffer[1] = 0x81;        // SET
	report_buffer[2] = addr & 0xFF; // Address LSB
	report_buffer[3] = addr >> 8;   // Address MSB
	*(uint32_t*)(&report_buffer[4]) = value; // Value

	DEBUG_PRINT(("Sending SET command.\n"));
	if (TdHidSetReport(context->handle, report_buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_OUTPUT))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	DEBUG_PRINT(("Listening SETA reply.\n"));
	if (TdHidListenReport(context->handle, report_buffer, REPORT_SIZE + 1) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	if (report_buffer[1] != 0x81 || ((report_buffer[3] << 8) | report_buffer[2]) != addr)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid reply.");

	return;
}


static int write(td_context_t* context)
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

		write_devreg(context, addr, value);
	}

	return 0;
}


static uint32_t read_devreg(td_context_t *context, uint16_t addr)
{
	unsigned char report_buffer[REPORT_SIZE + 1];
	memset(report_buffer, 0, REPORT_SIZE + 1);

	DEBUG_PRINT(("Sending GET command.\n"));
	report_buffer[0] = 0x00;        // Dummy report Id
	report_buffer[1] = 0x80;        // GET
	report_buffer[2] = addr & 0xFF; // Address LSB
	report_buffer[3] = addr >> 8;   // Address MSB
	if (TdHidSetReport(context->handle, report_buffer, REPORT_SIZE + 1, USB_HID_REPORT_TYPE_OUTPUT) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	// listen for GETA
	DEBUG_PRINT(("Listening GETA reply.\n"));
	if (TdHidListenReport(context->handle, report_buffer, REPORT_SIZE + 1) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	if( report_buffer[1] != 0x80 || ( (report_buffer[3] << 8) | report_buffer[2] ) != addr )
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid reply.");

	return *(uint32_t *)(&report_buffer[4]);
}


static int read(td_context_t* context)
{
	if (context->c == 0) // Default behavier of read operation
	{
		context->c = 1;
		context->v[0] = REGNAME_GPIO_STATUS;
	}

	for(int i=0 ; i < context->c ; i++)
	{
		uint16_t addr = devreg_name2addr(context->v[i]);
		uint32_t value = read_devreg(context, addr);

		if (context->format == FORMAT_RAW || context->format == FORMAT_SIMPLE)
		{
			printf("%d\n", value);
		}
		else
		{
			throw_exception(EXITCODE_INVALID_FORMAT, "Unknown format");
		}
	}

	return 0;
}


static int listen(td_context_t* context)
{
	unsigned char report_buffer[REPORT_SIZE + 1];
	memset(report_buffer, 0, REPORT_SIZE + 1);

	if (TdHidListenReport(context->handle, report_buffer, REPORT_SIZE + 1) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	printf("%d,%d\n", report_buffer[1], report_buffer[2]);

	return 0;
}

static td_device_t *export_type(void)
{
	td_device_t *device = (td_device_t *)malloc(sizeof(td_device_t));
	memset(device, 0, sizeof(td_device_t));

	device->vendor_id = TOKYODEVICES_VENDOR_ID;
	device->product_id = 6000;	
	device->output_report_size = REPORT_SIZE;
	device->input_report_size = REPORT_SIZE;
	device->capability1 = CPBLTY1_DFU_AFTER_DESTROY | CPBLTY1_SAVE_EEPROM;
	device->read = read;
	device->write = write;
	device->listen = listen;	
	
	return device;
}

td_device_t *(*tdfa30608_import)(void) = export_type;
