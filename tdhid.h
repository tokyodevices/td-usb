#pragma once

#define USBOPEN_SUCCESS					0   /* no error */
#define USBOPEN_ERR_ACCESS				1   /* not enough permissions to open device */
#define USBOPEN_ERR_IO					2   /* I/O error */
#define USBOPEN_ERR_NOTFOUND			3   /* device not found */

#define USB_HID_REPORT_TYPE_INPUT		0x01
#define USB_HID_REPORT_TYPE_OUTPUT		0x02
#define USB_HID_REPORT_TYPE_FEATURE		0x03

#define USBRQ_HID_GET_REPORT			0x01
#define USBRQ_HID_SET_REPORT			0x09

#define DEFAULT_TIMEOUT					1000


int TdHidListDevices(uint16_t vendor, uint16_t product, const char *name, char *lpBuffer, int szBuffer);
int *TdHidOpenDevice(uint16_t vendor_id, uint16_t product_id, const char *name, char *serial);
void TdHidCloseDevice(int *handle);
int TdHidSetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type);
int TdHidGetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type);
int TdHidListenReport(int *handle, unsigned char *buffer, int len);
