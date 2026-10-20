/**
* @file device_types.c
* @author s-dz, Tokyo Devices, Inc. (tokyodevices.jp)
* @date 2020-12-2
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "td-usb.h"

extern td_device_t* (*tdfa50507_import)(void);
extern td_device_t* (*tdfa30604_import)(void);
extern td_device_t* (*tdfa30608_import)(void);
extern td_device_t* (*iwt120_import)(void);
extern td_device_t* (*iws660_import)(void);
extern td_device_t* (*iws73x_import)(void);
extern td_device_t* (*tdpc0201_import)(void);
extern td_device_t* (*iwt303_import)(void);
extern td_device_t* (*iwt313_import)(void);
extern td_device_t* (*iwt1320_import)(void);
extern td_device_t* (*tdsn450_import)(void);
extern td_device_t* (*tdfa60220_import)(void);
extern td_device_t* (*tdfa60250_import)(void);
extern td_device_t* (*tdfa6032_import)(void);
extern td_device_t* (*tdpc0205_import)(void);
extern td_device_t* (*tdpc2401_import)(void);
extern td_device_t* (*tdpw6620_import)(void);
extern td_device_t* (*tdsn0700_import)(void);
extern td_device_t* (*tdfa60350_import)(void);
extern td_device_t* (*tdsn5200_import)(void);
extern td_device_t* (*tdsn7200_import)(void);
extern td_device_t* (*tdsn604xx_import)(void);
extern td_device_t* (*tdsn7400_import)(void);
extern td_device_t* (*tdsn7360_import)(void);
extern td_device_t* (*tdsn7502_import)(void);
extern td_device_t* (*tdfa1104_import)(void);
extern td_device_t* (*tdfa50607_import)(void);
extern td_device_t* (*tdsn7300_import)(void);
extern td_device_t* (*iws600cm_import)(void);
extern td_device_t* (*tdfa30301_import)(void);
extern td_device_t* (*tdfa30303_import)(void);
extern td_device_t* (*tdfa60228_import)(void);

static int model_name_matches(const char *model_name, int length, const char *expected)
{
	return length > 0 && strlen(expected) == (size_t)length &&
		strncmp(model_name, expected, (size_t)length) == 0;
}

td_device_t *import_device_type(const char *model_name, int compare_length)
{
	if (model_name_matches(model_name, compare_length, "tdfa50507")) return tdfa50507_import();
	if (model_name_matches(model_name, compare_length, "tdfa30604")) return tdfa30604_import();
	if (model_name_matches(model_name, compare_length, "tdfa30608")) return tdfa30608_import();
	if (model_name_matches(model_name, compare_length, "tdpc0201")) return tdpc0201_import();
	if (model_name_matches(model_name, compare_length, "iwt120")) return iwt120_import();
	if (model_name_matches(model_name, compare_length, "iws660")) return iws660_import();
	if (model_name_matches(model_name, compare_length, "iws73x")) return iws73x_import();
	if (model_name_matches(model_name, compare_length, "iwt303")) return iwt303_import();
	if (model_name_matches(model_name, compare_length, "iwt313")) return iwt313_import();
	if (model_name_matches(model_name, compare_length, "iwt1320")) return iwt1320_import();
	if (model_name_matches(model_name, compare_length, "tdsn450")) return tdsn450_import();
	if (model_name_matches(model_name, compare_length, "tdfa60220")) return tdfa60220_import();
	if (model_name_matches(model_name, compare_length, "tdfa60250")) return tdfa60250_import();
	if (model_name_matches(model_name, compare_length, "tdfa6032")) return tdfa6032_import();
	if (model_name_matches(model_name, compare_length, "tdpc0205")) return tdpc0205_import();
	if (model_name_matches(model_name, compare_length, "tdpc2401")) return tdpc2401_import();
	if (model_name_matches(model_name, compare_length, "tdpw6620")) return tdpw6620_import();
	if (model_name_matches(model_name, compare_length, "tdsn0700")) return tdsn0700_import();
	if (model_name_matches(model_name, compare_length, "tdfa60350")) return tdfa60350_import();
	if (model_name_matches(model_name, compare_length, "tdsn5200")) return tdsn5200_import();
	if (model_name_matches(model_name, compare_length, "tdsn7200")) return tdsn7200_import();
	if (model_name_matches(model_name, compare_length, "tdsn604xx")) return tdsn604xx_import();
	if (model_name_matches(model_name, compare_length, "tdsn7400")) return tdsn7400_import();
	if (model_name_matches(model_name, compare_length, "tdsn7360")) return tdsn7360_import();
	if (model_name_matches(model_name, compare_length, "tdsn7502")) return tdsn7502_import();
	if (model_name_matches(model_name, compare_length, "tdfa1104")) return tdfa1104_import();
	if (model_name_matches(model_name, compare_length, "tdfa50607")) return tdfa50607_import();
	if (model_name_matches(model_name, compare_length, "tdsn7300")) return tdsn7300_import();
	if (model_name_matches(model_name, compare_length, "iws600cm")) return iws600cm_import();
	if (model_name_matches(model_name, compare_length, "tdfa30301")) return tdfa30301_import();
	if (model_name_matches(model_name, compare_length, "tdfa30303")) return tdfa30303_import();
	if (model_name_matches(model_name, compare_length, "tdfa60228")) return tdfa60228_import();
	return 0;
}

void delete_device_type(td_device_t *device_type)
{
	if (device_type)
	{
		if (device_type->product_name)
		{
			free(device_type->product_name);
			device_type->product_name = NULL;
		}
		free(device_type);
		device_type = NULL;
	}
}
