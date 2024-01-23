// tdhid-libusb.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <errno.h>
#include "tdhid.h"

static int didUsbInit = 0;

static int usbhidGetStringAscii(usb_dev_handle *dev, int index, char *buf, int buflen)
{
	char    buffer[256];
	int     rval, i;

	if ((rval = usb_get_string_simple(dev, index, buf, buflen)) >= 0) /* use libusb version if it works */
		return rval;
	if ((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, 0x0409, buffer, sizeof(buffer), 5000)) < 0)
		return rval;
	if (buffer[1] != USB_DT_STRING) {
		*buf = 0;
		return 0;
	}
	if ((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0];
	rval /= 2;
	/* lossy conversion to ISO Latin1: */
	for (i = 1; i<rval; i++) {
		if (i > buflen)              /* destination buffer overflow */
			break;
		buf[i - 1] = buffer[2 * i];
		if (buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
			buf[i - 1] = '?';
	}
	buf[i - 1] = 0;
	return i - 1;
}

int TdHidListDevices(uint16_t vendor, uint16_t product, const char *product_name, char *lpBuffer, int szBuffer)
{
	char tmp_buffer[256]; // For USB devices, the maximum string length is 126 wide characters (not including the terminating NULL character). (by MSDN)
	int	buffer_count = 0;

	struct usb_bus      *bus;
	struct usb_device   *dev;
	usb_dev_handle      *handle = NULL;

	if (lpBuffer != NULL && szBuffer > 0) lpBuffer[0] = '\0';

	if (!didUsbInit)
	{
		usb_init();
		didUsbInit = 1;
	}

	usb_find_busses();

	usb_find_devices();

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			//fprintf(stdout, "Got device: [%d]:[%d]\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
			if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product)
			{
				int     len;

				handle = usb_open(dev);

				if (!handle)
				{
					fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
				}
				else
				{
					len = usbhidGetStringAscii(handle, dev->descriptor.iSerialNumber, tmp_buffer, sizeof(tmp_buffer));

					if (len < 0)
					{
						fprintf(stderr, "Warning: cannot query product for device: %s\n", usb_strerror());
					}
					else
					{
						//fprintf(stdout, "Got serial: [%s]\n", buffer_count, tmp_buffer);

						if (buffer_count > 0)
						{
							buffer_count += 1;
							if (lpBuffer != NULL && szBuffer >= (buffer_count + 1)) strcat(lpBuffer, ",");
						}

						buffer_count += len;
						if (lpBuffer != NULL && szBuffer >= (buffer_count + 1)) strcat(lpBuffer, tmp_buffer);
					}

					usb_close(handle);
					handle = NULL;
				}
			}
		}
	}

	return (buffer_count + 1); // +1 ... NULL terminator.
}

int *TdHidOpenDevice(uint16_t vendor, uint16_t product, const char *product_name, char *serial)
{
	int ret;
	struct usb_bus      *bus;
	struct usb_device   *dev;
	usb_dev_handle      *handle = NULL;

	if (!didUsbInit)
	{
		usb_init();
		didUsbInit = 1;
	}

	usb_find_busses();

	usb_find_devices();

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product)
			{
				char    string[256];
				int     len;

				handle = usb_open(dev);

				if (!handle)
				{
					fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
					continue;
				}

				if (product_name != NULL)
				{
					len = usbhidGetStringAscii(handle, dev->descriptor.iProduct, string, sizeof(string));

					if (len < 0)
					{
						fprintf(stderr, "Warning: cannot query ProductName for device: %s\n", usb_strerror());
					}
					else
					{
						//fprintf(stdout, "Got ProductName: %s\n", string);
						if (strcmp(string, product_name) != 0) 
						{
							usb_close(handle);
							handle = NULL;
							continue;
						}
					}
				}
				
				if (serial != NULL)
				{
					len = usbhidGetStringAscii(handle, dev->descriptor.iSerialNumber, string, sizeof(string));

					if (len < 0)
					{
						fprintf(stderr, "Warning: cannot query SerialNumber for device: %s\n", usb_strerror());
					}
					else
					{
						//fprintf(stdout, "Got serial: %s\n", string);
						if (strcmp(string, serial) != 0) {
							usb_close(handle);
							handle = NULL;
							continue;
						}
					}
				}

				goto FOUND; // target is found.
			}
		}
	}

FOUND:
	// Claim interface #0
	if (handle != NULL && dev->config->bNumInterfaces > 0) {
		ret = usb_claim_interface(handle, 0);
		if (ret < 0) {
			usb_detach_kernel_driver_np(handle, 0);
			ret = usb_claim_interface(handle, 0);
			if (ret < 0) {
				fprintf(stderr, "Could not claim interface #0\n");
				return NULL;
			}
		}
	}

	return (int *)handle;
}

void TdHidCloseDevice(int *handle)
{
	if (handle != NULL) usb_close((usb_dev_handle *)handle);
}

int TdHidGetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type)
{
	int bytesReceived;

	bytesReceived = usb_control_msg(
		(void *)handle,
		USB_TYPE_CLASS | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
		USBRQ_HID_GET_REPORT,
		report_type << 8,
		0,
		(char *)&buffer[1],
		len - 1,
		DEFAULT_TIMEOUT);

	if (bytesReceived < 0)
	{
		if (bytesReceived == -ETIMEDOUT) {
			fprintf(stderr, "Timeout\n");
		}
		else {
			fprintf(stderr, "Error on receiving report: %s\n", usb_strerror());
		}
		return USBOPEN_ERR_IO;
	}

	return 0;
}

int TdHidListenReport(int *handle, unsigned char *buffer, int len)
{
	int bytesReceived;

	// via EP#1 (interrupt transfer)
	bytesReceived = usb_interrupt_read((void *)handle, 0x81, (char *)&buffer[1], len-1, 0);

	if (bytesReceived < 0)
	{
		if (bytesReceived == -ETIMEDOUT) 
		{
			fprintf(stderr, "Timeout\n");
			return 2;
		}
		else
		{
			fprintf(stderr, "Error on receiving report: %s\n", usb_strerror());
			return 1;
		}
	}

	return 0;
}

int TdHidSetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type)
{
	int bytesSent;

	bytesSent = usb_control_msg(
		(void *)handle,
		USB_TYPE_CLASS | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
		USBRQ_HID_SET_REPORT, report_type << 8, 0,
		(char *)&buffer[1], len-1, DEFAULT_TIMEOUT);

	if (bytesSent != len-1) {
		if (bytesSent < 0) {
			if (bytesSent == -ETIMEDOUT) {
				fprintf(stderr, "Timeout\n");
			}
			else {
				fprintf(stderr, "Error on sending report: %s\n", usb_strerror());
			}
		}
		return USBOPEN_ERR_IO;
	}
	return 0;
}
