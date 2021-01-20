// device_types.c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "td-usb.h"

extern td_device_t *(*tdfa30608_import)(void);
extern td_device_t *(*iwt120_import)(void);
extern td_device_t *(*iws660_import)(void);

td_device_t *import_device_type(const char *model_name, int compare_length)
{
	if (!strncmp(model_name, "tdfa30608", compare_length)) return tdfa30608_import();
	if (!strncmp(model_name, "iwt120", compare_length)) return iwt120_import();
	if (!strncmp(model_name, "iws660", compare_length)) return iws660_import();
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
