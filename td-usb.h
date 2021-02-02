#pragma once

#define __STDC_WANT_LIB_EXT1__

#define TOKYODEVICES_VENDOR_ID					13038 // 0x32EE

#define OPERATION_LIST						0
#define OPERATION_LISTEN					1
#define OPERATION_READ						2
#define OPERATION_WRITE						3
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

#define MAX_REPORT_LENGTH						64

#define CPBLTY1_DFU_MASK						3
#define CPBLTY1_DFU_NOT_AVAILABLE				0
#define CPBLTY1_DFU_AFTER_DESTROY				1
#define CPBLTY1_DFU_AFTER_SWITCH				2

#define CPBLTY1_CHANGE_SERIAL					4
#define CPBLTY1_SAVE_EEPROM						8

#define TD_CONTEXT_MAX_ARG_COUNT				16


#ifdef _DEBUG
#define DEBUG_PRINT(arg)    printf arg
#else
#define DEBUG_PRINT(arg)
#endif


typedef struct {
	int* handle;
	int interval;
	uint8_t operation;
	uint8_t format;
	uint8_t loop;
	char *v[TD_CONTEXT_MAX_ARG_COUNT];
	int c;
} td_context_t;


typedef struct {
	char *product_name;
	unsigned short vendor_id;
	unsigned short product_id;
	uint8_t output_report_size;
	uint8_t input_report_size;
	int (*write)(td_context_t* context);
	int (*read)(td_context_t* context);
	int (*listen)(td_context_t* context);
	uint8_t capability1;
} td_device_t;



td_device_t *import_device_type(const char *model_name, int compare_length);
void delete_device_type(td_device_t *device_type);
void throw_exception(int exitcode, const char *msg);
