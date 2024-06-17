// tdhid-win32.c
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <setupapi.h>
#include <api/hidsdi.h>
#include <api/hidpi.h>

#include "td-usb.h"
#include "tdhid.h"



static void print_last_error_msg()
{
#ifdef _DEBUG
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	fprintf(stderr, "Error. Code=%d ", GetLastError());
	fprintf(stderr, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
#endif
}

void convertUniToAscii(char *buffer)
{
	unsigned short  *uni = (unsigned short *)buffer;
	char            *ascii = buffer;
	while (*uni != 0) *ascii++ = (*uni >= 256) ? '?' : ((char)*uni++);
	*ascii++ = 0;
}

int open_device(LPHANDLE lpHandle, GUID *pGUID, HDEVINFO deviceInfoList, int i, UINT16 vendor_id, UINT16 product_id)
{
	HIDD_ATTRIBUTES                     deviceAttributes;
	SP_DEVICE_INTERFACE_DATA            deviceInfo;
	DWORD                               size;
	SP_DEVICE_INTERFACE_DETAIL_DATA     *deviceDetails = NULL;
	BOOLEAN								bResult;

	deviceInfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (!SetupDiEnumDeviceInterfaces(deviceInfoList, NULL, pGUID, i, &deviceInfo)) {
		*lpHandle = INVALID_HANDLE_VALUE;
		return 1;
	}

	SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, NULL, 0, (PDWORD)&size, NULL);
	deviceDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(size);
	if (deviceDetails == NULL)
	{
		DEBUG_PRINT(("  Open failed.\n"));
		return 4;
	}
	deviceDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, deviceDetails, size, (PDWORD)&size, NULL);
	DEBUG_PRINT(("  Path: \"%s\"\n", deviceDetails->DevicePath));

	*lpHandle = CreateFile(deviceDetails->DevicePath, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
//		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	free(deviceDetails);

	if (*lpHandle == INVALID_HANDLE_VALUE)
	{
		DEBUG_PRINT(("  Open failed.\n"));
		return 2;
	}

	deviceAttributes.Size = sizeof(HIDD_ATTRIBUTES);
	bResult = HidD_GetAttributes(*lpHandle, &deviceAttributes);
	if (bResult == FALSE)
	{
		DEBUG_PRINT(("  Open failed.\n"));
		return 5;
	}
	DEBUG_PRINT(("  Attributes: vid=%d pid=%d\n", deviceAttributes.VendorID, deviceAttributes.ProductID));

	if (deviceAttributes.VendorID != vendor_id || deviceAttributes.ProductID != product_id) {
		CloseHandle(*lpHandle);
		*lpHandle = INVALID_HANDLE_VALUE;
		return 3;
	}

	return 0;
}

int TdHidListDevices(UINT16 vendor_id, UINT16 product_id, LPCSTR product_name, LPSTR lpBuffer, int szBuffer)
{
	char tmp_buffer[128]; // For USB devices, the maximum string length is 126 wide characters (not including the terminating NULL character). (by MSDN)
	GUID                                hidGuid;
	HDEVINFO							deviceInfoList;
	int									buffer_count = 0;
	int result;
	HANDLE handle;

	HidD_GetHidGuid(&hidGuid);
	deviceInfoList = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

	if (lpBuffer != NULL && szBuffer > 0) lpBuffer[0] = '\0';

	for (int i = 0; ; i++)
	{
		DEBUG_PRINT(("Device#%d\n", i));

		result = open_device(&handle, &hidGuid, deviceInfoList, i, vendor_id, product_id);
		if (result == 1) {
			DEBUG_PRINT(("  No such device.\n"));
			break;
		}
		if (result != 0) {
			continue;
		}

		DEBUG_PRINT(("  VID/PID matched.\n"));

		if (product_name != NULL)
		{
			result = HidD_GetProductString(handle, tmp_buffer, sizeof(tmp_buffer));
			if (result == 0)
			{
				CloseHandle(handle);
				continue;
			}

			DEBUG_PRINT(("  ProductString: \"%ls\"\n", (unsigned short *)tmp_buffer));

			convertUniToAscii(tmp_buffer);

			if (strcmp(product_name, tmp_buffer) != 0)
			{
				CloseHandle(handle);
				continue;
			}

			DEBUG_PRINT(("  ProductName matched.\n"));
		}

		result = HidD_GetSerialNumberString(handle, tmp_buffer, sizeof(tmp_buffer));

		CloseHandle(handle);

		if (result == 0) continue;

		DEBUG_PRINT(("  SerialNumber: \"%ls\"\n", (unsigned short *)tmp_buffer));

		convertUniToAscii(tmp_buffer);

		if (buffer_count > 0) {
			buffer_count += 1; // comma length
			if (lpBuffer != NULL) strcat_s(lpBuffer, szBuffer, ",");
		}

		buffer_count += strlen(tmp_buffer);
		if (lpBuffer != NULL) strcat_s(lpBuffer, szBuffer, tmp_buffer);
	}

	SetupDiDestroyDeviceInfoList(deviceInfoList);

	return (buffer_count + 1); // +1 = NULL
}


int *TdHidOpenDevice(UINT16 vendor_id, UINT16 product_id, LPCSTR product_name, LPSTR serialNumber)
{
	char tmp_buffer[128]; // For USB devices, the maximum string length is 126 wide characters (not including the terminating NULL character). (by MSDN)
	int *handle = INVALID_HANDLE_VALUE;
	GUID   hidGuid;
	HDEVINFO deviceInfoList;

	HidD_GetHidGuid(&hidGuid);
	deviceInfoList = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

	for (int i = 0;; i++)
	{
		int result;

		DEBUG_PRINT(("Device#%d\n", i));

		if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);

		result = open_device(&handle, &hidGuid, deviceInfoList, i, vendor_id, product_id);
		if (result == 1) break; // no device found anymore
		if (result != 0) continue;

		if (product_name != NULL)
		{
			result = HidD_GetProductString(handle, tmp_buffer, sizeof(tmp_buffer));
			if (result == 0)
			{
				CloseHandle(handle);
				handle = INVALID_HANDLE_VALUE;
				continue;
			}

			DEBUG_PRINT(("  ProductString: \"%ls\"\n", (unsigned short *)tmp_buffer));

			convertUniToAscii(tmp_buffer);

			if (strcmp(product_name, tmp_buffer) != 0)
			{
				CloseHandle(handle);
				handle = INVALID_HANDLE_VALUE;
				continue;
			}
		}

		if (serialNumber != NULL)
		{
			result = HidD_GetSerialNumberString(handle, tmp_buffer, sizeof(tmp_buffer));
			if (result == 0)
			{
				CloseHandle(handle);
				handle = INVALID_HANDLE_VALUE;
				continue;
			}

			DEBUG_PRINT(("  SerialNumber: \"%ls\"\n", (unsigned short *)tmp_buffer));

			convertUniToAscii(tmp_buffer);

			if (strcmp(serialNumber, tmp_buffer) != 0) {
				CloseHandle(handle);
				handle = INVALID_HANDLE_VALUE;
				continue;
			}
		}

		break; // target found
	}

	SetupDiDestroyDeviceInfoList(deviceInfoList);

	return (handle != INVALID_HANDLE_VALUE) ? handle : NULL;
}


void TdHidCloseDevice(int *handle)
{
	if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
}


int TdHidSetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type)
{
	OVERLAPPED overlapped;
	DWORD nWrite = 0;
	BOOLEAN result = 0;
	
	if (report_type == USB_HID_REPORT_TYPE_FEATURE) 
	{
		result = HidD_SetFeature(handle, buffer, len);
	}
	else if (report_type == USB_HID_REPORT_TYPE_OUTPUT) 
	{
		result = HidD_SetOutputReport(handle, buffer, len);
	}
	else 
	{
		fprintf(stderr, "SET_REPORT with bmReportType=Input is invalid.\n");
	}

	if (result == 0) {
		print_last_error_msg();
		return USBOPEN_ERR_IO;
	} else {
		return TDHID_SUCCESS;
	}	
}


int TdHidGetReport(int *handle, unsigned char *buffer, int len, uint8_t report_type)
{
	BOOLEAN result;

	if (report_type == USB_HID_REPORT_TYPE_FEATURE)
		result = HidD_GetFeature(handle, buffer, len);
	else if (report_type == USB_HID_REPORT_TYPE_INPUT)
		result = HidD_GetInputReport(handle, buffer, len);
	else
		result = FALSE;

	if (result == FALSE) {
		print_last_error_msg();
		return 2;
	}
	else {
		return TDHID_SUCCESS;
	}
}


int TdHidListenReport(int *handle, unsigned char *buffer, int len)
{
	OVERLAPPED overlapped;
	DWORD nRead = 0;

	memset(&overlapped, 0, sizeof(overlapped));

	if ( ReadFile(handle, (LPVOID)buffer, len, &nRead, &overlapped) == FALSE )
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			if ( WaitForSingleObject(handle, DEFAULT_TIMEOUT) != WAIT_OBJECT_0 )
			{				
				CancelIo(handle);
				return TDHID_ERR_TIMEOUT; // Timeout
			}
		}
		else
		{
			print_last_error_msg();
			return TDHID_ERR_IO;
		}
	}
	
	return TDHID_SUCCESS;
}
