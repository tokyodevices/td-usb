/* Exercise the Windows discovery adapter without opening any USB device. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum failure {
    NONE, QUERY_ERROR, QUERY_SUCCESS, QUERY_ZERO_SIZE, DETAILS_ERROR,
    ALLOCATION_ERROR, OPEN_ERROR, ATTRIBUTES_ERROR, WRONG_ID
};
static enum failure failure;
static int device_count, serial_length, create_calls, close_calls, invalid_calls;
static int allocation_calls, active_allocations, allocated_count;
static HANDLE allocated_handles[64];

static void WINAPI mock_hid_guid(LPGUID guid) { memset(guid, 0, sizeof(*guid)); }
static HDEVINFO WINAPI mock_class_devs(const GUID* a, PCSTR b, HWND c, DWORD d)
{ (void)a; (void)b; (void)c; (void)d; return (HDEVINFO)(UINT_PTR)1; }
static BOOL WINAPI mock_enumerate(HDEVINFO a, PSP_DEVINFO_DATA b, const GUID* c,
    DWORD index, PSP_DEVICE_INTERFACE_DATA data)
{
    (void)a; (void)b; (void)c;
    assert(data->cbSize == sizeof(*data));
    if (index < (DWORD)device_count) return TRUE;
    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
}
static BOOL WINAPI mock_detail(HDEVINFO a, PSP_DEVICE_INTERFACE_DATA b,
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A detail, DWORD size, PDWORD needed, PSP_DEVINFO_DATA c)
{
    (void)a; (void)b; (void)c;
    if (!detail) {
        assert(size == 0);
        if (failure == QUERY_ERROR) { SetLastError(ERROR_NO_SUCH_DEVICE); return FALSE; }
        *needed = failure == QUERY_ZERO_SIZE ? 0 : 64;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return failure == QUERY_SUCCESS ? TRUE : FALSE;
    }
    assert(size == 64 && detail->cbSize == sizeof(*detail));
    if (failure == DETAILS_ERROR) { SetLastError(ERROR_NO_SUCH_DEVICE); return FALSE; }
    strcpy_s(detail->DevicePath, size - offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath), "test-only");
    return TRUE;
}
static HANDLE WINAPI mock_create_file(LPCSTR path, DWORD access, DWORD share,
    LPSECURITY_ATTRIBUTES security, DWORD disposition, DWORD flags, HANDLE template_file)
{
    (void)share; (void)security; (void)disposition; (void)template_file;
    create_calls++;
    assert(failure != QUERY_ERROR && failure != QUERY_SUCCESS && failure != QUERY_ZERO_SIZE);
    assert(failure != DETAILS_ERROR && failure != ALLOCATION_ERROR);
    assert(strcmp(path, "test-only") == 0);
    assert(access == (GENERIC_READ | GENERIC_WRITE) && (flags & FILE_FLAG_OVERLAPPED));
    if (failure == OPEN_ERROR) { SetLastError(ERROR_ACCESS_DENIED); return INVALID_HANDLE_VALUE; }
    HANDLE handle = CreateEventA(NULL, TRUE, FALSE, NULL);
    assert(handle && allocated_count < 64);
    allocated_handles[allocated_count++] = handle;
    return handle;
}
static BOOLEAN WINAPI mock_attributes(HANDLE handle, PHIDD_ATTRIBUTES attributes)
{
    (void)handle;
    assert(attributes->Size == sizeof(*attributes));
    if (failure == ATTRIBUTES_ERROR) { SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE; }
    attributes->VendorID = failure == WRONG_ID ? 0 : 0x32EE;
    attributes->ProductID = 1;
    return TRUE;
}
static BOOLEAN WINAPI mock_serial(HANDLE handle, PVOID output, ULONG size)
{
    (void)handle;
    if (size < (ULONG)(serial_length + 1) * sizeof(WCHAR)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    WCHAR* text = output;
    for (int i = 0; i < serial_length; i++) text[i] = L'1';
    text[serial_length] = 0;
    return TRUE;
}
static BOOLEAN WINAPI mock_product(HANDLE handle, PVOID output, ULONG size)
{
    (void)handle;
    static const WCHAR product[] = L"Test";
    assert(size >= sizeof(product));
    memcpy(output, product, sizeof(product));
    return TRUE;
}
static BOOL WINAPI mock_destroy_list(HDEVINFO info) { (void)info; return TRUE; }
static BOOL WINAPI mock_close(HANDLE handle)
{
    int found = 0;
    for (int i = 0; i < allocated_count; i++) {
        if (allocated_handles[i] == handle) {
            allocated_handles[i] = NULL;
            found = 1;
            break;
        }
    }
    assert(found); /* Also detect a second close of the same handle. */
    close_calls++;
    return CloseHandle(handle);
}
static void* mock_malloc(size_t size)
{
    allocation_calls++;
    if (failure == ALLOCATION_ERROR) return NULL;
    void* pointer = malloc(size);
    assert(pointer);
    memset(pointer, 0xA5, size);
    active_allocations++;
    return pointer;
}
static void mock_free(void* pointer)
{
    if (pointer) { assert(active_allocations > 0); active_allocations--; }
    free(pointer);
}
static void mock_invalid_parameter(const wchar_t* a, const wchar_t* b,
    const wchar_t* c, unsigned int d, uintptr_t e)
{ (void)a; (void)b; (void)c; (void)d; (void)e; invalid_calls++; }

#define HidD_GetHidGuid mock_hid_guid
#define SetupDiGetClassDevsA mock_class_devs
#define SetupDiEnumDeviceInterfaces mock_enumerate
#define SetupDiGetDeviceInterfaceDetailA mock_detail
#define CreateFileA mock_create_file
#define HidD_GetAttributes mock_attributes
#define HidD_GetProductString mock_product
#define HidD_GetSerialNumberString mock_serial
#define SetupDiDestroyDeviceInfoList mock_destroy_list
#define CloseHandle mock_close
#define malloc mock_malloc
#define free mock_free
#include "../windows/tdhid-windows.c"
#undef CloseHandle
#undef malloc
#undef free

void debug_print(const char* message, ...) { (void)message; }

static void assert_no_resources(void)
{
    assert(active_allocations == 0);
    for (int i = 0; i < allocated_count; i++) assert(allocated_handles[i] == NULL);
}
static void reset(void)
{
    assert_no_resources();
    failure = NONE;
    device_count = 1;
    serial_length = 4;
    create_calls = close_calls = invalid_calls = allocation_calls = allocated_count = 0;
}

static void test_conversion(void)
{
    unsigned short input[] = { 'A', 0xFF, 0x100, 'B', 0 };
    const unsigned char expected[] = { 'A', 0xFF, '?', 'B', 0 };
    convertUniToAscii((char*)input);
    assert(memcmp(input, expected, sizeof(expected)) == 0);

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    char* pages = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages);
    DWORD old_protection;
    assert(VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old_protection));
    char* buffer = pages + info.dwPageSize - 128;
    memset(buffer, 0, 128);
    ((unsigned short*)buffer)[0] = 0x0100;
    DWORD caught_status = 0;
    __try { convertUniToAscii(buffer); }
    __except (EXCEPTION_EXECUTE_HANDLER) { caught_status = GetExceptionCode(); }
    assert(caught_status == 0 && strcmp(buffer, "?") == 0 && buffer[127] == 0);
    VirtualFree(pages, 0, MEM_RELEASE);
}

static void test_list_capacity(void)
{
    for (int capacity = 0; capacity <= 11; capacity++) {
        reset();
        device_count = 2;
        unsigned char storage[13];
        memset(storage, 0xCC, sizeof(storage));
        char* output = (char*)storage + 1;
        assert(TdHidListDevices(0x32EE, 1, NULL, output, capacity) == 10);
        assert(storage[0] == 0xCC && storage[capacity + 1] == 0xCC);
        if (capacity >= 10) assert(strcmp(output, "1111,1111") == 0);
        else if (capacity > 0) assert(output[0] == 0);
        assert(invalid_calls == 0);
        assert_no_resources();
    }
    reset();
    device_count = 0;
    assert(TdHidListDevices(0x32EE, 1, NULL, NULL, 0) == 1);
    char output[1];
    device_count = 1; /* A new device between the size query and result retrieval. */
    assert(TdHidListDevices(0x32EE, 1, NULL, output, sizeof(output)) == 5);
    assert(output[0] == 0 && invalid_calls == 0);
    assert_no_resources();
}

static void test_long_serials(void)
{
    const int lengths[] = { 0, 63, 64, 126 };
    for (unsigned int i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        reset();
        serial_length = lengths[i];
        char output[128], expected[128];
        memset(expected, '1', serial_length);
        expected[serial_length] = 0;
        assert(TdHidListDevices(0x32EE, 1, "Test", NULL, 0) == serial_length + 1);
        assert(TdHidListDevices(0x32EE, 1, "Test", output, sizeof(output)) == serial_length + 1);
        assert(strcmp(output, expected) == 0);
        int* handle = TdHidOpenDevice(0x32EE, 1, "Test", expected);
        assert(handle != NULL);
        TdHidCloseDevice(handle);
        assert_no_resources();
    }
}

static void test_open_failures(void)
{
    for (int mode = QUERY_ERROR; mode <= WRONG_ID; mode++) {
        reset();
        failure = (enum failure)mode;
        GUID guid = { 0 };
        HANDLE output = (HANDLE)(UINT_PTR)0xDEAD;
        assert(open_device(&output, &guid, (HDEVINFO)(UINT_PTR)1, 0, 0x32EE, 1) != 0);
        assert(output == INVALID_HANDLE_VALUE);
        if (mode <= ALLOCATION_ERROR) assert(create_calls == 0);
        if (mode <= QUERY_ZERO_SIZE) assert(allocation_calls == 0);
        assert_no_resources();
        assert(TdHidListDevices(0x32EE, 1, NULL, NULL, 0) == 1);
        assert(TdHidOpenDevice(0x32EE, 1, NULL, NULL) == NULL);
        assert_no_resources();
    }
    reset();
    assert(TdHidOpenDevice(0x32EE, 1, "Other", NULL) == NULL);
    assert(TdHidOpenDevice(0x32EE, 1, NULL, "Other") == NULL);
    assert(create_calls == close_calls);
    assert_no_resources();
}

int main(void)
{
    _invalid_parameter_handler previous = _set_invalid_parameter_handler(mock_invalid_parameter);
    test_conversion();
    test_list_capacity();
    test_long_serials();
    test_open_failures();
    _set_invalid_parameter_handler(previous);
    puts("PASS: Unicode conversion, list capacity/hotplug, 126-character serials and discovery failure cleanup");
    return 0;
}
