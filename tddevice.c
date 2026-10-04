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
#include "tdtimer.h"


static uint8_t buffer[MAX_REPORT_LENGTH + 1];

int tddev2_wait_response(td_context_t* context, uint8_t* report, const uint8_t* expected, int expected_len)
{
	uint64_t deadline = TdTimer_GetMilliseconds() + DEFAULT_TIMEOUT;

	while (1)
	{
		uint64_t now = TdTimer_GetMilliseconds();
		if (now >= deadline) return TDHID_ERR_TIMEOUT;

		int result = TdHidReadInputReport(context->handle, report, context->device_type->input_report_size + 1,
			(int)(deadline - now));
		if (result != TDHID_SUCCESS) return result;
		if (memcmp(&report[1], expected, expected_len) == 0) return TDHID_SUCCESS;
	}
}

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
		throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
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
		throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);


	const uint8_t expected[] = { INPACKET_ACK, OUTPACKET_SAVE };
	int result = tddev2_wait_response(context, buffer, expected, sizeof(expected));
	if (result != TDHID_SUCCESS)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, result == TDHID_ERR_TIMEOUT ? ERROR_MSG_DEVICE_TIMEOUT : ERROR_MSG_DEVICE_IO_ERROR);

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
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
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
		throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

	// Older firmware sends ACK without the command byte for SET.
	const uint8_t expected[] = { INPACKET_ACK };
	int result = tddev2_wait_response(context, buffer, expected, sizeof(expected));
	if (result != TDHID_SUCCESS)
		throw_exception(EXITCODE_DEVICE_IO_ERROR, result == TDHID_ERR_TIMEOUT ? ERROR_MSG_DEVICE_TIMEOUT : ERROR_MSG_DEVICE_IO_ERROR);

	return 0;
}


/**
* @brief TDDEV2 Std. read devreg
*/
uint32_t tddev2_read_devreg(td_context_t* context, uint16_t addr)
{
	const uint8_t expected[] = { INPACKET_DEVREG, addr & 0xFF, addr >> 8 };

	for (int retry_count = 0; retry_count < 3; retry_count++)
	{
		DEBUG_PRINT((">> OUTPACKET_GET (ADDR: 0x%02X)\n", addr));
		memset(buffer, 0, MAX_REPORT_LENGTH + 1);
		buffer[1] = OUTPACKET_GET;
		buffer[2] = addr & 0xFF;
		buffer[3] = addr >> 8;

		if (TdHidSetReport(context->handle, buffer, context->device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT))
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);

		int result = tddev2_wait_response(context, buffer, expected, sizeof(expected));
		if (result == TDHID_SUCCESS) return *(uint32_t*)(&buffer[4]);
		if (result != TDHID_ERR_TIMEOUT)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_IO_ERROR);
	}

	throw_exception(EXITCODE_DEVICE_IO_ERROR, ERROR_MSG_DEVICE_TIMEOUT);
	return 0;
}
