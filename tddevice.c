/**
* @file tddevice.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
* @date 2021-3-20
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "td-usb.h"
#include "tdhid.h"
#include "tddevice.h"


static uint8_t buffer[MAX_REPORT_LENGTH + 1];

/**
* @brief TDDEV1 Std. INIT Command
*/
int tddev1_init_operation(td_context_t* context)
{
	time_t epoc;
	
	memset(buffer, 0, MAX_REPORT_LENGTH + 1);

	buffer[1] = TDDEV1_CMD_INIT;

	time(&epoc); sprintf((char*)&buffer[2], "%llu", epoc);

	if (TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_FEATURE))
	{
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
	}

	printf("Set serial number to %s\n", &buffer[2]);
	
	return 0;
}

/**
* @brief TDDEV2 Std. SAVE
*/
int tddev2_save_to_flash(td_context_t* context)
{
	memset(buffer, 0, MAX_REPORT_LENGTH + 1);

	buffer[1] = OUTPACKET_SAVE; // OUTPACKET_SAVE
	buffer[2] = 0x50; // Magic

	DEBUG_PRINT(("Sending SAVE command.\n"));
	if (TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");


	// listen for ACK
	DEBUG_PRINT(("Listening ACK reply.\n"));
	while (1)
	{
		if (TdHidListenReport(context->handle, buffer, context->device_type->input_report_size + 1) != 0)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
		if (buffer[1] == INPACKET_ACK && buffer[2] == OUTPACKET_SAVE) break;
	}

	printf("Device registers have been saved to flash.\n");

	return 0;
}


/**
* @brief TDDEV2 Std. destroy
*/
int tddev2_destroy_firmware(td_context_t* context)
{
	printf("WARNING: The device will not be available until new firmware is written. Continue? [y/N]");
	char c = fgetc(stdin);

	if (c == 'y' || c == 'Y')
	{
		memset(buffer, 0, MAX_REPORT_LENGTH + 1);
		buffer[1] = OUTPACKET_ERASE; buffer[2] = 0x31; buffer[3] = 0x1C; buffer[4] = 0x66; // ERASE command & magics
		if (TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT))
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
	}
	else
	{
		printf("abort.\n");
	}

	return 0;
}


/**
* @brief TDDEV2 Std. write devreg
*/
int tddev2_write_devreg(td_context_t* context, uint16_t addr, uint32_t value)
{
	memset(buffer, 0, MAX_REPORT_LENGTH + 1);

	buffer[0] = 0x00;				 // Dummy report Id
	buffer[1] = OUTPACKET_SET;       // OUTPACKET_SET
	buffer[2] = addr & 0xFF; // Address LSB
	buffer[3] = addr >> 8;   // Address MSB
	*(uint32_t*)(&buffer[4]) = value; // Value

	DEBUG_PRINT(("Sending SET command.\n"));
	if (TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT))
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	// listen for ACK
	DEBUG_PRINT(("Listening ACK reply.\n"));
	while (1)
	{
		if (TdHidListenReport(context->handle, buffer, context->device_type->input_report_size + 1) != 0)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
		if (buffer[1] == INPACKET_ACK) break;
	}

	return 0;
}


/**
* @brief TDDEV2 Std. read devreg
*/
uint32_t tddev2_read_devreg(td_context_t* context, uint16_t addr)
{
	memset(buffer, 0, MAX_REPORT_LENGTH + 1);

	DEBUG_PRINT(("Sending GET command.\n"));
	buffer[0] = 0x00;        // Dummy report Id
	buffer[1] = OUTPACKET_GET; // OUTPACKET_GET
	buffer[2] = addr & 0xFF; // Address LSB
	buffer[3] = addr >> 8;   // Address MSB
	if (TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT) != 0)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

	// listen for INPACKET_DEVREG
	DEBUG_PRINT(("Listening INPACKET_DEVREG\n"));
	while (1)
	{
		if (TdHidListenReport(context->handle, buffer, context->device_type->input_report_size + 1) != 0)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
		if (buffer[1] == INPACKET_DEVREG || ((buffer[3] << 8) | buffer[2]) == addr) break;
	}

	return *(uint32_t*)(&buffer[4]);
}
