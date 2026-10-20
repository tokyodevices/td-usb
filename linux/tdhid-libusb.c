// tdhid-libusb.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <errno.h>
#include "../tdhid.h"

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

static int refresh_devices(void)
{
	if (!didUsbInit)
	{
		usb_init();
		didUsbInit = 1;
	}

	if (usb_find_busses() < 0 || usb_find_devices() < 0)
	{
		fprintf(stderr, "Could not enumerate USB devices: %s\n", usb_strerror());
		return -USBOPEN_ERR_IO;
	}
	return 0;
}

static int matches_string(usb_dev_handle *handle, int index, const char *expected, const char *name)
{
	char string[256];
	if (expected == NULL) return 1;

	if (usbhidGetStringAscii(handle, index, string, sizeof(string)) < 0)
	{
		fprintf(stderr, "Warning: cannot query %s for device: %s\n", name, usb_strerror());
		return 0;
	}
	return strcmp(string, expected) == 0;
}

static usb_dev_handle *open_candidate(struct usb_device *dev, const char *product_name, const char *serial)
{
	int saved_errno;
	usb_dev_handle *handle = usb_open(dev);
	if (handle == NULL)
	{
		fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
		return NULL;
	}

	if (!matches_string(handle, dev->descriptor.iProduct, product_name, "ProductName") ||
		!matches_string(handle, dev->descriptor.iSerialNumber, serial, "SerialNumber"))
		goto fail;

	if (dev->descriptor.bNumConfigurations == 0 || dev->config == NULL || dev->config->bNumInterfaces == 0)
	{
		fprintf(stderr, "USB device has no usable configuration/interface\n");
		errno = ENODEV;
		goto fail;
	}

	// Claim interface #0, detaching the kernel driver if necessary.
	if (usb_claim_interface(handle, 0) < 0)
	{
		usb_detach_kernel_driver_np(handle, 0);
		if (usb_claim_interface(handle, 0) < 0)
		{
			fprintf(stderr, "Could not claim interface #0: %s\n", usb_strerror());
			goto fail;
		}
	}
	return handle;

fail:
	saved_errno = errno;
	usb_close(handle);
	errno = saved_errno;
	return NULL;
}

int TdHidListDevices(uint16_t vendor, uint16_t product, const char *product_name, char *lpBuffer, int szBuffer)
{
	char tmp_buffer[256];
	int buffer_count = 0;
	struct usb_bus *bus;
	struct usb_device *dev;

	if (lpBuffer != NULL && szBuffer > 0) lpBuffer[0] = '\0';
	if (refresh_devices() < 0) return -USBOPEN_ERR_IO;

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor != vendor || dev->descriptor.idProduct != product) continue;

			usb_dev_handle *handle = usb_open(dev);
			if (handle == NULL)
			{
				fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
				continue;
			}

			if (matches_string(handle, dev->descriptor.iProduct, product_name, "ProductName"))
			{
				int len = usbhidGetStringAscii(handle, dev->descriptor.iSerialNumber, tmp_buffer, sizeof(tmp_buffer));
				if (len < 0)
					fprintf(stderr, "Warning: cannot query SerialNumber for device: %s\n", usb_strerror());
				else
				{
					if (buffer_count > 0)
					{
						buffer_count += 1;
						if (lpBuffer != NULL && szBuffer >= (buffer_count + 1)) strcat(lpBuffer, ",");
					}
					buffer_count += len;
					if (lpBuffer != NULL && szBuffer >= (buffer_count + 1)) strcat(lpBuffer, tmp_buffer);
				}
			}
			usb_close(handle);
		}
	}
	return buffer_count + 1; // Include the NUL terminator.
}

int *TdHidOpenDevice(uint16_t vendor, uint16_t product, const char *product_name, char *serial)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	if (refresh_devices() < 0) return NULL;

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor != vendor || dev->descriptor.idProduct != product) continue;
			usb_dev_handle *handle = open_candidate(dev, product_name, serial);
			if (handle != NULL) return (int *)handle;
		}
	}
	return NULL;
}

void TdHidCloseDevice(int *handle)
{
	if (handle != NULL) usb_close((usb_dev_handle *)handle);
}

int TdHidGetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type)
{
	int bytesReceived;
	if (len < 2) return USBOPEN_ERR_IO;
	buffer[0] = 0; // Dummy report ID; not present on the USB wire.

	bytesReceived = usb_control_msg(
		(void *)handle,
		USB_TYPE_CLASS | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
		USBRQ_HID_GET_REPORT,
		report_type << 8,
		0,
		(char *)&buffer[1],
		len - 1,
		DEFAULT_TIMEOUT);

	if (bytesReceived != len - 1)
	{
		if (bytesReceived == -ETIMEDOUT) {
			fprintf(stderr, "Timeout\n");
		}
		else if (bytesReceived < 0) {
			fprintf(stderr, "Error on receiving report: %s\n", usb_strerror());
		}
		else {
			fprintf(stderr, "Short report received: %d of %d bytes\n", bytesReceived, len - 1);
		}
		return USBOPEN_ERR_IO;
	}

	return 0;
}

int TdHidReadInputReport(int *handle, unsigned char *buffer, int len, int timeout_ms)
{
	if (len < 2 || timeout_ms < 0) return TDHID_ERR_IO;
	buffer[0] = 0; // Dummy report ID; not present on the USB wire.

	int bytesReceived = usb_interrupt_read((void *)handle, 0x81, (char *)&buffer[1], len - 1, timeout_ms);
	if (bytesReceived == -ETIMEDOUT) return TDHID_ERR_TIMEOUT;
	if (bytesReceived != len - 1) return TDHID_ERR_IO;
	return TDHID_SUCCESS;
}

int TdHidSetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type)
{
	int bytesSent;

	bytesSent = usb_control_msg(
		(void *)handle,
		USB_TYPE_CLASS | USB_ENDPOINT_OUT |
			(report_type == USB_HID_REPORT_TYPE_OUTPUT ? USB_RECIP_INTERFACE : USB_RECIP_DEVICE),
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
