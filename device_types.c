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

td_device_t *import_device_type(const char *model_name, int compare_length)
{
	if (!strncmp(model_name, "tdfa50507", compare_length)) return tdfa50507_import();
	if (!strncmp(model_name, "tdfa30604", compare_length)) return tdfa30604_import();
	if (!strncmp(model_name, "tdfa30608", compare_length)) return tdfa30608_import();
	if (!strncmp(model_name, "tdpc0201", compare_length)) return tdpc0201_import();
	if (!strncmp(model_name, "iwt120", compare_length)) return iwt120_import();
	if (!strncmp(model_name, "iws660", compare_length)) return iws660_import();
	if (!strncmp(model_name, "iws73x", compare_length)) return iws73x_import();
	if (!strncmp(model_name, "iwt303", compare_length)) return iwt303_import();
	if (!strncmp(model_name, "iwt313", compare_length)) return iwt313_import();
	if (!strncmp(model_name, "iwt1320", compare_length)) return iwt1320_import();
	if (!strncmp(model_name, "tdsn450", compare_length)) return tdsn450_import();
	if (!strncmp(model_name, "tdfa60220", compare_length)) return tdfa60220_import();
	if (!strncmp(model_name, "tdfa60250", compare_length)) return tdfa60250_import();
	if (!strncmp(model_name, "tdfa6032", compare_length)) return tdfa6032_import();
	if (!strncmp(model_name, "tdpc0205", compare_length)) return tdpc0205_import();
	if (!strncmp(model_name, "tdsn0700", compare_length)) return tdsn0700_import();
	if (!strncmp(model_name, "tdfa60350", compare_length)) return tdfa60350_import();
	if (!strncmp(model_name, "tdsn5200", compare_length)) return tdsn5200_import();
	if (!strncmp(model_name, "tdsn7200", compare_length)) return tdsn7200_import();
	if (!strncmp(model_name, "tdsn604xx", compare_length)) return tdsn604xx_import();
	if (!strncmp(model_name, "tdsn7400", compare_length)) return tdsn7400_import();
	if (!strncmp(model_name, "tdsn7360", compare_length)) return tdsn7360_import();
	if (!strncmp(model_name, "tdsn7502", compare_length)) return tdsn7502_import();
	if (!strncmp(model_name, "tdfa1104", compare_length)) return tdfa1104_import();
	if (!strncmp(model_name, "tdfa50607", compare_length)) return tdfa50607_import();
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
