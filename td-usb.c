/**
* @file tdusb.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
* @date 2020-12-2
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "td-usb.h"
#include "tdtimer.h"
#include "tdhid.h"
#include "tddevice.h"
#include "tdthread.h"

static td_context_t *context = NULL;


void sigint_handler(int sig) 
{
	throw_exception(0, NULL);
}

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

		if (context->device_type != NULL) {
			delete_device_type(context->device_type);
			context->device_type = NULL;
		}

		free(context);
		context = NULL;
	}

	exit(exitcode);
}


static void print_usage(void)
{
	printf("TD-USB version 0.3.4\n");
	printf("Copyright (C) 2020-2025 Tokyo Devices, Inc. (tokyodevices.jp)\n");
	printf("Usage: td-usb model_name[:serial] operation [options]\n");
	printf("Visit https://github.com/tokyodevices/td-usb/ for details\n");
}

void debug_print(const char* msg, ...)
{
	if (context->verbose == TRUE)
	{
		va_list args;
		va_start(args, msg);
		vprintf(msg, args);
		va_end(args);
	}
}

static void parse_args(int argc, char *argv[])
{
	char *arg_operation;
	char *arg_model_name;
	int arg_model_name_length;
	char *p;

	context->format = FORMAT_SIMPLE;
	context->skip = 0;
	context->interval = 0;
	context->loop = FALSE;

	if (argc < 3) 
	{ 
		print_usage(); 
		exit(1); 
	}

	arg_model_name = argv[1];
	p = strchr(arg_model_name, ':');
	arg_model_name_length = (p == NULL) ? strlen(arg_model_name) : (p - arg_model_name);

	// Get device type object by model name string
	context->device_type = import_device_type(arg_model_name, arg_model_name_length);
	if (context->device_type == NULL)
	{
		fprintf(stderr, "Unknown model name: %s\n", arg_model_name);
		throw_exception(EXITCODE_UNKNOWN_DEVICE, NULL);
	}

	arg_operation = argv[2];
	if (!strcmp(arg_operation, "list")) context->operation = OPERATION_LIST;
	else if (!strcmp(arg_operation, "listen")) context->operation = OPERATION_LISTEN;
	else if (!strcmp(arg_operation, "get")) context->operation = OPERATION_GET;
	else if (!strcmp(arg_operation, "set")) context->operation = OPERATION_SET;
	else if (!strcmp(arg_operation, "save")) context->operation = OPERATION_SAVE;
	else if (!strcmp(arg_operation, "destroy")) context->operation = OPERATION_DESTROY;
	else if (!strcmp(arg_operation, "dfu")) context->operation = OPERATION_DFU;
	else if (!strcmp(arg_operation, "init")) context->operation = OPERATION_INIT;
	else
	{
		fprintf(stderr, "Unknown operation: %s\n", arg_operation);
		throw_exception(EXITCODE_UNKNOWN_OPERATION, NULL);
	}

	context->c = 0;
	context->v[0] = NULL;

	for (int i = 3; i < argc; i++)
	{
		if ( strlen(argv[i]) >= 2 && (argv[i][0] == '-' && argv[i][1] == '-') )
		{
			if (strncmp("--format", argv[i], 8) == 0)
			{
				p = strchr(argv[i], '=');
				if (p == NULL)
				{
					fprintf(stderr, "No format is specified.\n");
					throw_exception(EXITCODE_INVALID_OPTION, NULL);
				}
				if (strcmp("json", p + 1) == 0) context->format = FORMAT_JSON;
				if (strcmp("raw", p + 1) == 0) context->format = FORMAT_RAW;
				if (strcmp("csv", p + 1) == 0) context->format = FORMAT_CSV;
				if (strcmp("tsv", p + 1) == 0) context->format = FORMAT_TSV;
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
			else if (strncmp("--skip", argv[i], 6) == 0)
			{
				p = strchr(argv[i], '=');
				if (p == NULL)
				{
					fprintf(stderr, "Skip count is not set.\n");
					throw_exception(EXITCODE_INVALID_OPTION, NULL);
				}

				context->skip = atoi(p + 1);
				if (context->skip < OPTION_MIN_SKIP)
				{
					fprintf(stderr, "Skip count must be >= %d.\n", OPTION_MIN_SKIP);
					throw_exception(EXITCODE_INVALID_OPTION, NULL);
				}
			}
			else if (strncmp("--verbose", argv[i], 9) == 0)
			{
				context->verbose = TRUE;
			}
		}
		else
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


int main(int argc, char *argv[])
{
	char *p;

	setvbuf(stdout, NULL, _IONBF, 0);
	
	signal(SIGINT, sigint_handler);

	context = (td_context_t *)malloc(sizeof(td_context_t));
	memset(context, 0, sizeof(td_context_t));
	parse_args(argc, argv);

	if (context->operation == OPERATION_LIST)
	{
		int len = TdHidListDevices(
			context->device_type->vendor_id,
			context->device_type->product_id,
			context->device_type->product_name,
			NULL, 0);
		char* p = (char*)malloc(len);
		TdHidListDevices(
			context->device_type->vendor_id,
			context->device_type->product_id,
			context->device_type->product_name,
			p, len);	
		printf("%s\n",p);
		free(p);		
		throw_exception(EXITCODE_NO_ERROR, NULL);
	}

	p = strchr(argv[1], ':');
	context->handle = TdHidOpenDevice(
		context->device_type->vendor_id,
		context->device_type->product_id,
		context->device_type->product_name,
		(p == NULL) ? NULL : p + 1);
	if (context->handle == NULL)
	{
		fprintf(stderr, "Device open error. code=%d\n", errno);
		throw_exception(EXITCODE_DEVICE_OPEN_ERROR, NULL);
	}
	
	switch (context->operation)
	{
	case OPERATION_LISTEN:
		if (context->device_type->listen != NULL)
		{
			do
			{
				context->device_type->listen(context);
			} while (context->loop == TRUE);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"Listen operation is not supported on this device.");
		}
		break;

	case OPERATION_GET:		
		if (context->device_type->get != NULL)
		{
			context->device_type->get(context);
			if (context->interval > 0)	TdTimer_Start(context->device_type->get, context, context->interval);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"GET operation is not supported on this device.");
		}
		break;

	case OPERATION_SET:
		if (context->device_type->set != NULL)
		{
			if (context->loop == TRUE && context->c == 0)
			{
				char buffer[TD_SETLOOP_BUFFER_SIZE];

				while (TRUE)
				{
					if (fgets(buffer, TD_SETLOOP_BUFFER_SIZE, stdin) == NULL) break;

					buffer[strcspn(buffer, "\n")] = '\0';

					context->c = 0;
					char* token = strtok(buffer, " ");
					while (token != NULL && context->c < TD_CONTEXT_MAX_ARG_COUNT) {
						context->v[context->c] = token;
						context->c++;
						token = strtok(NULL, " ");
					}

					context->device_type->set(context);
				}
			}
			else
			{
				context->device_type->set(context);
			}
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"SET operation is not supported on this device.");
		}
		break;

	case OPERATION_SAVE:
		if (context->device_type->save != NULL)
		{
			context->device_type->save(context);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"SAVE operation is not supported on this device.");
		}
		break;

	case OPERATION_DESTROY:
		if (context->device_type->destroy != NULL)
		{
			context->device_type->destroy(context);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"DESTROY operation is not supported on this device.");
		}
		break;

	case OPERATION_DFU:
		throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
			"Switching to DFU-mode is not supported yet.");
		break;

	case OPERATION_INIT:
		if (context->device_type->init != NULL)
		{
			context->device_type->init(context);
		}
		else
		{
			throw_exception(EXITCODE_OPERATION_NOT_SUPPORTED,
				"INIT operation is not supported on this device.");
		}		
		break;
	}

	throw_exception(EXITCODE_NO_ERROR, NULL);
}
