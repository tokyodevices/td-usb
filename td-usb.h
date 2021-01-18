#pragma once

#define __STDC_WANT_LIB_EXT1__

#define TOKYODEVICES_VENDOR_ID			13038 // 0x32EE

#define OPTION_FORMAT_SIMPLE			0
#define OPTION_FORMAT_RAW				1
#define OPTION_FORMAT_JSON				2
#define OPTION_FORMAT_CSV				3
#define OPTION_FORMAT_TSV				4

#define OPTION_DEFAULT_INTERVAL			1000

#define	FALSE							0
#define	TRUE							1

#define EXITCODE_NO_ERROR				0
#define EXITCODE_UNKNOWN_DEVICE			2
#define EXITCODE_UNKNOWN_OPERATION		3
#define EXITCODE_DEVICE_OPEN_ERROR		11
#define EXITCODE_DEVICE_IO_ERROR		12

#define MAX_REPORT_LENGTH				64

#define CPBLTY1_DFU_MASK				3
#define CPBLTY1_DFU_NOT_AVAILABLE		0
#define CPBLTY1_DFU_AFTER_ERASE			1
#define CPBLTY1_DFU_AFTER_SWITCH		2

#define CPBLTY1_CHANGE_SERIAL			4

typedef struct {
	char *product_name;
	unsigned short vendor_id;
	unsigned short product_id;
	uint8_t report_size;
	uint8_t input_report_type;
	uint8_t output_report_type;
	int (*prepare_report)(int format, const char *report_string, uint8_t *buffer);
	int (*print_report)(int format, uint8_t *buffer);
	uint8_t capability1;
	uint8_t capability2; // reserved
} td_device_t;

td_device_t *import_device_type(const char *model_name, int compare_length);
void delete_device_type(td_device_t *device_type);
void errExit(int exitcode, const char *msg);

