// tdusb.c

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "td-usb.h"
#include "tdtimer.h"
#include "tdhid.h"
#include "tdthread.h"

static td_context_t *context = NULL;
static td_device_t *device_type = NULL;

/**
* @brief Exit process with specified error code and message.
* @param[in] exitcode Exit code
* @param[in] msg Error message
*/
void throw_exception(int exitcode, const char *msg)
{
	if (msg != NULL) fprintf(stderr, "%s\n", msg);

	if (context != NULL)
	{
		if (context->handle != NULL) {
			TdHidCloseDevice(context->handle);
			context->handle = NULL;
		}

		free(context);
		context = NULL;
	}

	if (device_type != NULL) {
		delete_device_type(device_type);
		device_type = NULL;
	}

	exit(exitcode);
}


/**
* @brief
*/
static void tddev2_save_to_flash(void)
{
	uint8_t buffer[MAX_REPORT_LENGTH + 1];

	if ((device_type->capability1 & CPBLTY1_SAVE_EEPROM) != 0)
	{
		memset(buffer, 0, device_type->output_report_size + 1);
		buffer[1] = 0xF1; // SAVE
		buffer[2] = 0x50; // Magic

		DEBUG_PRINT(("Sending SAVE command.\n"));
		if (TdHidSetReport(context->handle, buffer, device_type->output_report_size + 1, USB_HID_REPORT_TYPE_OUTPUT))
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

		DEBUG_PRINT(("Listening SAVEA reply.\n"));
		if (TdHidListenReport(context->handle, buffer, device_type->input_report_size + 1) != 0)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");

		if (buffer[1] != 0xF1)
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "Save failure.");

		printf("Device registers have been saved to flash.\n");
	}
	else
	{
		throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
			"Save operation is not supported on this device.");
	}
}


/**
* @brief
*/
static void tddev2_destroy_firmware(void)
{
	uint8_t* buffer = NULL;

	if ((device_type->capability1 & CPBLTY1_DFU_MASK) == CPBLTY1_DFU_AFTER_DESTROY)
	{
		printf("WARNING: The device will not be available until new firmware is written. Continue? [y/N]");
		char c = fgetc(stdin);

		if (c == 'y')
		{
			if (device_type->output_report_size > 0 &&
				device_type->output_report_size + 1 < MAX_REPORT_LENGTH)
			{
				buffer = malloc(device_type->output_report_size + 1);
				memset(buffer, 0, device_type->output_report_size + 1);
				buffer[1] = 0xF6; buffer[2] = 0x31; buffer[3] = 0x1C; buffer[4] = 0x66; // ERASE command & magics

				if (TdHidSetReport(context->handle,
					buffer, 
					device_type->output_report_size + 1,
					USB_HID_REPORT_TYPE_OUTPUT))
				{
					free(buffer); buffer = NULL;
					throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
				}

				free(buffer); buffer = NULL;
			}
			else
			{
				throw_exception(EXITCODE_DEVICE_IO_ERROR, "Invalid size of output report.");
			}
		}
		else
		{
			printf("abort.\n");
		}
	}
	else
	{
		throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
			"Erase operation is not supported on this device.");
	}
}



/**
* @brief
*/
static void tddev2_switch_to_dfu(void)
{
	if ((device_type->capability1 & CPBLTY1_DFU_MASK) == CPBLTY1_DFU_AFTER_SWITCH)
	{
		fprintf(stderr, "Switching to DFU-mode is not supported yet.\n");
	}
	else
	{
		throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
			"Switching to DFU-mode is not supported on this device.");
	}
}


/**
* @brief
*/
static void tddev1_init_operation(void)
{
	uint8_t buffer[MAX_REPORT_LENGTH + 1];

	// 0x82 INIT Command for old-device. need report size == 16.
	if (device_type->capability1 & CPBLTY1_CHANGE_SERIAL)
	{
		time_t epoc;

		memset(buffer, 0, device_type->output_report_size + 1);
		buffer[1] = 0x82;
		time(&epoc); sprintf((char*)&buffer[2], "%llu", epoc);

		if (TdHidSetReport(context->handle, buffer, device_type->output_report_size + 1, USB_HID_REPORT_TYPE_FEATURE))
		{
			throw_exception(EXITCODE_DEVICE_IO_ERROR, "USB I/O Error.");
		}

		printf("Set serial number to %s\n", &buffer[2]);
	}
	else
	{
		throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
			"Changing serial is not supported on this device.");
	}
}


static void listen_worker(void *p)
{
	while(1) device_type->listen(p);
}


static void print_usage(void)
{
	printf("td-usb version 0.2\n");
	printf("Copyright (C) 2020-2021 Tokyo Devices, Inc. (tokyodevices.jp)\n");
	printf("Usage: td-usb model_name[:serial] operation [options]\n");
	printf("Visit https://github.com/tokyodevices/td-usb/ for details\n");
}

static void parse_args(int argc, char *argv[])
{
	char *arg_operation;
	char *arg_model_name;
	int arg_model_name_length;
	char *p;

	context->format = FORMAT_SIMPLE;

	if (argc < 3) 
	{ 
		print_usage(); 
		exit(1); 
	}

	arg_model_name = argv[1];
	p = strchr(arg_model_name, ':');
	arg_model_name_length = (p == NULL) ? strlen(arg_model_name) : (p - arg_model_name);

	// Get device type object by model name string
	device_type = import_device_type(arg_model_name, arg_model_name_length);
	if (device_type == NULL)
	{
		fprintf(stderr, "Unknown model name: %s\n", arg_model_name);
		throw_exception(EXITCODE_UNKNOWN_DEVICE, NULL);
	}

	arg_operation = argv[2];
	if (!strcmp(arg_operation, "list")) context->operation = OPERATION_LIST;
	else if (!strcmp(arg_operation, "listen")) context->operation = OPERATION_LISTEN;
	else if (!strcmp(arg_operation, "read")) context->operation = OPERATION_READ;
	else if (!strcmp(arg_operation, "write")) context->operation = OPERATION_WRITE;
	else if (!strcmp(arg_operation, "save")) context->operation = OPERATION_SAVE;
	else if (!strcmp(arg_operation, "destroy")) context->operation = OPERATION_DESTROY;
	else if (!strcmp(arg_operation, "dfu")) context->operation = OPERATION_DFU;
	else if (!strcmp(arg_operation, "init")) context->operation = OPERATION_INIT;
	else
	{
		fprintf(stderr, "Unknown operation: %s\n", arg_operation);
		throw_exception(EXITCODE_UNKNOWN_OPERATION, NULL);
	}

	if (context->operation == OPERATION_READ ||
		context->operation == OPERATION_WRITE ||
		context->operation == OPERATION_LISTEN)
	{
		context->c = 0;
		context->v[0] = NULL;

		for (int i = 3; i < argc; i++)
		{
			if (strlen(argv[i]) < 2) continue;

			if (argv[i][0] == '-' && argv[i][1] == '-') // options
			{
				if (strncmp("--format", argv[i], 8) == 0)
				{
					p = strchr(argv[i], '=');
					if (p != NULL)
					{
						if (strcmp("json", p + 1) == 0) context->format = FORMAT_JSON;
						if (strcmp("raw", p + 1) == 0) context->format = FORMAT_RAW;
						if (strcmp("csv", p + 1) == 0) context->format = FORMAT_CSV; // reserved.
						if (strcmp("tsv", p + 1) == 0) context->format = FORMAT_TSV; // reserved.
					}
				}
				else if (strncmp("--loop", argv[i], 6) == 0)
				{
					context->loop = TRUE;

					p = strchr(argv[i], '=');
					if (p != NULL)
					{
						context->interval = atoi(p + 1);
						if (context->interval < OPTION_MIN_INTERVAL)
						{
							fprintf(stderr, "Interval must be >= %d msec.\n", OPTION_MIN_INTERVAL);
							throw_exception(EXITCODE_INVALID_OPTION, NULL);
						}
					}
					else
					{
						context->interval = OPTION_DEFAULT_INTERVAL;
					}
				}
			}
			else // property name|ids
			{
				if (context->c >= TD_CONTEXT_MAX_ARG_COUNT)
				{
					fprintf(stderr, "Option count must be < %d\n", TD_CONTEXT_MAX_ARG_COUNT);
					throw_exception(EXITCODE_INVALID_OPTION, NULL);
				}
				else
				{
					context->v[context->c] = argv[i];
					context->c++;
					DEBUG_PRINT(("OPTION:"));
					DEBUG_PRINT((argv[i]));
					DEBUG_PRINT(("\n"));
				}
			}
		}
	}
}


int main(int argc, char *argv[])
{
	char *p;

	context = (td_context_t *)malloc(sizeof(td_context_t));
	parse_args(argc, argv);

	if (context->operation == OPERATION_LIST)
	{
		int len = TdHidListDevices(
			device_type->vendor_id,
			device_type->product_id,
			device_type->product_name,
			NULL, 0);
		char* p = (char*)malloc(len);
		TdHidListDevices(
			device_type->vendor_id,
			device_type->product_id,
			device_type->product_name,
			p, len);
		printf("%s\n",p);
		free(p);
		throw_exception(EXITCODE_NO_ERROR, NULL);
	}

	p = strchr(argv[1], ':');
	context->handle = TdHidOpenDevice(
		device_type->vendor_id,
		device_type->product_id, 
		device_type->product_name,
		(p == NULL) ? NULL : p + 1);
	if (context->handle == NULL)
	{
		fprintf(stderr, "Device open error. code=%d\n", errno);
		throw_exception(EXITCODE_DEVICE_OPEN_ERROR, NULL);
	}
	
	switch (context->operation)
	{
	case OPERATION_LISTEN:
		if (device_type->listen != NULL)
		{
			do
			{
				device_type->listen(context);
			} while (context->loop == TRUE);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"Listen operation is not supported on this device.");
		}
		break;

	case OPERATION_READ:		
		if (device_type->read != NULL)
		{
			device_type->read(context);
			if (context->interval > 0)	TdTimer_Start(device_type->read, context, context->interval);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"Read operation is not supported on this device.");
		}
		break;

	case OPERATION_WRITE:
		if (device_type->write != NULL)
		{
			device_type->write(context);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"Write operation is not supported on this device.");
		}
		break;

	case OPERATION_SAVE:
		tddev2_save_to_flash();
		break;

	case OPERATION_DESTROY:
		tddev2_destroy_firmware();
		break;

	case OPERATION_DFU:
		tddev2_switch_to_dfu();
		break;

	case OPERATION_INIT:
		tddev1_init_operation();
		break;
	}

	throw_exception(EXITCODE_NO_ERROR, NULL);
}
