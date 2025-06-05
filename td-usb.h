#pragma once

#define __STDC_WANT_LIB_EXT1__

#define TOKYODEVICES_VENDOR_ID				13038 // 0x32EE

#define OPERATION_LIST						0
#define OPERATION_LISTEN					1
#define OPERATION_GET						2
#define OPERATION_SET						3
#define OPERATION_SAVE						10
#define OPERATION_INIT						12
#define OPERATION_DFU						13
#define OPERATION_DESTROY					14


#define FORMAT_SIMPLE						0
#define FORMAT_RAW							1
#define FORMAT_JSON							2
#define FORMAT_CSV							3
#define FORMAT_TSV							4

#define OPTION_DEFAULT_INTERVAL					1000
#define OPTION_MIN_INTERVAL						50
#define OPTION_MIN_SKIP							1

#define	FALSE									0
#define	TRUE									1

#define EXITCODE_NO_ERROR						0
#define EXITCODE_UNKNOWN_DEVICE					2
#define EXITCODE_UNKNOWN_OPERATION				3
#define EXITCODE_OPERATION_NOT_SUPPORTED		4
#define EXITCODE_INVALID_OPTION					6
#define EXITCODE_DEVICE_OPEN_ERROR				11
#define EXITCODE_DEVICE_IO_ERROR				12
#define EXITCODE_INVALID_FORMAT					13
#define EXITCODE_INVALID_RANGE					14

#define ERROR_MSG_DEVICE_IO_ERROR				"USB I/O Error."
#define ERROR_MSG_INVALID_FORMAT				"Invalid format."

#define MAX_REPORT_LENGTH						64

#define TD_CONTEXT_MAX_ARG_COUNT				16
#define TD_SETLOOP_BUFFER_SIZE					256

#define DEBUG_PRINT(arg)    debug_print arg

// Forward declaration
typedef struct td_context_t td_context_t;

typedef struct {
	char* product_name;
	unsigned short vendor_id;
	unsigned short product_id;
	uint8_t output_report_size;
	uint8_t input_report_size;
	int (*set)(td_context_t* context);
	int (*save)(td_context_t* context);
	int (*get)(td_context_t* context);
	int (*listen)(td_context_t* context);
	int (*init)(td_context_t* context);
	int (*destroy)(td_context_t* context);	
} td_device_t;


struct td_context_t
{
	int* handle;
	td_device_t* device_type;
	int interval;
	int skip;
	uint8_t operation;
	uint8_t format;
	uint8_t loop;
	uint8_t verbose;
	char* v[TD_CONTEXT_MAX_ARG_COUNT];
	int c;
};



td_device_t *import_device_type(const char *model_name, int compare_length);
void delete_device_type(td_device_t *device_type);
void throw_exception(int exitcode, const char *msg);
void debug_print(const char* msg, ...);
