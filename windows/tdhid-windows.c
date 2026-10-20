// tdhid-windows.c
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>

#include "..\td-usb.h"
#include "..\tdhid.h"



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
	while (*uni != 0) {
		unsigned short c = *uni++;
		*ascii++ = (c >= 256) ? '?' : (char)c;
	}
	*ascii = '\0';
}

int open_device(LPHANDLE lpHandle, GUID *pGUID, HDEVINFO deviceInfoList, int i, UINT16 vendor_id, UINT16 product_id)
{
	HIDD_ATTRIBUTES                     deviceAttributes;
	SP_DEVICE_INTERFACE_DATA            deviceInfo = { 0 };
	DWORD                               size = 0;
	SP_DEVICE_INTERFACE_DETAIL_DATA     *deviceDetails = NULL;
	HANDLE                              handle = INVALID_HANDLE_VALUE;
	int                                 result = 6; // Interface detail error.

	*lpHandle = INVALID_HANDLE_VALUE;
	deviceInfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	if (!SetupDiEnumDeviceInterfaces(deviceInfoList, NULL, pGUID, i, &deviceInfo)) {
		return 1;
	}

	// A size query is expected to fail with ERROR_INSUFFICIENT_BUFFER.
	if (SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, NULL, 0, &size, NULL) ||
		GetLastError() != ERROR_INSUFFICIENT_BUFFER || size < sizeof(*deviceDetails))
		goto cleanup;

	deviceDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(size);
	if (deviceDetails == NULL)
	{
		DEBUG_PRINT(("  Open failed.\n"));
		result = 4;
		goto cleanup;
	}
	deviceDetails->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	if (!SetupDiGetDeviceInterfaceDetail(deviceInfoList, &deviceInfo, deviceDetails, size, &size, NULL))
		goto cleanup;
	DEBUG_PRINT(("  Path: \"%s\"\n", deviceDetails->DevicePath));

	handle = CreateFile(deviceDetails->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		DEBUG_PRINT(("  Open failed.\n"));
		result = 2;
		goto cleanup;
	}

	deviceAttributes.Size = sizeof(HIDD_ATTRIBUTES);
	if (!HidD_GetAttributes(handle, &deviceAttributes))
	{
		DEBUG_PRINT(("  Open failed.\n"));
		result = 5;
		goto cleanup;
	}
	DEBUG_PRINT(("  Attributes: vid=%d pid=%d\n", deviceAttributes.VendorID, deviceAttributes.ProductID));

	if (deviceAttributes.VendorID != vendor_id || deviceAttributes.ProductID != product_id) {
		result = 3;
		goto cleanup;
	}

	// Transfer ownership only after all checks succeed.
	*lpHandle = handle;
	handle = INVALID_HANDLE_VALUE;
	result = 0;

cleanup:
	free(deviceDetails);
	if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
	return result;
}

int TdHidListDevices(UINT16 vendor_id, UINT16 product_id, LPCSTR product_name, LPSTR lpBuffer, int szBuffer)
{
	char tmp_buffer[128 * sizeof(WCHAR)]; // Up to 126 UTF-16 characters plus the terminator.
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

		int separator_length = buffer_count > 0 ? 1 : 0;
		buffer_count += separator_length + (int)strlen(tmp_buffer);
		// Append a complete entry only when it and the terminator fit.
		if (lpBuffer != NULL && szBuffer > buffer_count) {
			if (separator_length) strcat_s(lpBuffer, szBuffer, ",");
			strcat_s(lpBuffer, szBuffer, tmp_buffer);
		}
	}

	SetupDiDestroyDeviceInfoList(deviceInfoList);

	// Do not expose a partial list if the device count grew between calls.
	if (lpBuffer != NULL && szBuffer > 0 && buffer_count >= szBuffer) lpBuffer[0] = '\0';
	return (buffer_count + 1); // +1 = NULL
}


int *TdHidOpenDevice(UINT16 vendor_id, UINT16 product_id, LPCSTR product_name, LPSTR serialNumber)
{
	char tmp_buffer[128 * sizeof(WCHAR)]; // Up to 126 UTF-16 characters plus the terminator.
	int *handle = INVALID_HANDLE_VALUE;
	GUID   hidGuid;
	HDEVINFO deviceInfoList;

	HidD_GetHidGuid(&hidGuid);
	deviceInfoList = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

	for (int i = 0;; i++)
	{
		int result;

		DEBUG_PRINT(("Device#%d\n", i));

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


int TdHidReadInputReport(int *handle, unsigned char *buffer, int len, int timeout_ms)
{
	OVERLAPPED overlapped = { 0 };
	DWORD nRead = 0;
	int result = TDHID_SUCCESS;

	if (len < 2 || timeout_ms < 0) return TDHID_ERR_IO;
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (overlapped.hEvent == NULL) return TDHID_ERR_IO;

	if (!ReadFile(handle, buffer, len, &nRead, &overlapped))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			DWORD wait = WaitForSingleObject(overlapped.hEvent, timeout_ms == 0 ? INFINITE : (DWORD)timeout_ms);
			if (wait != WAIT_OBJECT_0)
			{
				CancelIo(handle);
				// The buffer and OVERLAPPED must remain alive until cancellation completes.
				GetOverlappedResult(handle, &overlapped, &nRead, TRUE);
				result = wait == WAIT_TIMEOUT ? TDHID_ERR_TIMEOUT : TDHID_ERR_IO;
			}
			else if (!GetOverlappedResult(handle, &overlapped, &nRead, FALSE))
			{
				result = TDHID_ERR_IO;
			}
		}
		else
		{
			result = TDHID_ERR_IO;
		}
	}

	CloseHandle(overlapped.hEvent);
	if (result == TDHID_SUCCESS && nRead != (DWORD)len) return TDHID_ERR_IO;
	return result;
}
