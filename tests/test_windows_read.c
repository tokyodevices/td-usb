/* Run against a local overlapped pipe, without touching any USB device. */
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../tdhid.h"
#include "../tdtimer.h"

void debug_print(const char* message, ...) { (void)message; }

static const unsigned char packet[9] = { 0, 0xFF, 0xF1, 0, 0, 0, 0, 0, 0 };

static DWORD WINAPI delayed_write(void* handle)
{
    DWORD written;
    Sleep(DEFAULT_TIMEOUT + 100);
    assert(WriteFile(handle, packet, sizeof(packet), &written, NULL));
    assert(written == sizeof(packet));
    return 0;
}

int main(void)
{
    char name[128];
    sprintf(name, "\\\\.\\pipe\\td-usb-receive-test-%lu", GetCurrentProcessId());
    HANDLE reader = CreateNamedPipeA(name, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_WAIT, 1, 64, 64, 0, NULL);
    assert(reader != INVALID_HANDLE_VALUE);
    HANDLE writer = CreateFileA(name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    assert(writer != INVALID_HANDLE_VALUE);
    OVERLAPPED connection = { 0 };
    connection.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    assert(connection.hEvent != NULL);
    assert(!ConnectNamedPipe(reader, &connection) && GetLastError() == ERROR_PIPE_CONNECTED);
    CloseHandle(connection.hEvent);

    unsigned char buffer[9];
    uint64_t start = TdTimer_GetMilliseconds();
    assert(TdHidReadInputReport((int*)reader, buffer, sizeof(buffer), 50) == TDHID_ERR_TIMEOUT);
    assert(TdTimer_GetMilliseconds() - start >= 40);

    /* A timed-out read must be fully cancelled before the buffer is reused. */
    DWORD written;
    assert(WriteFile(writer, packet, sizeof(packet), &written, NULL));
    assert(TdHidReadInputReport((int*)reader, buffer, sizeof(buffer), 500) == TDHID_SUCCESS);
    assert(memcmp(buffer, packet, sizeof(packet)) == 0);

    HANDLE thread = CreateThread(NULL, 0, delayed_write, writer, 0, NULL);
    assert(thread != NULL);
    start = TdTimer_GetMilliseconds();
    assert(TdHidReadInputReport((int*)reader, buffer, sizeof(buffer), 0) == TDHID_SUCCESS);
    assert(TdTimer_GetMilliseconds() - start >= DEFAULT_TIMEOUT);
    assert(memcmp(buffer, packet, sizeof(packet)) == 0);
    assert(WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0);
    CloseHandle(thread);

    assert(WriteFile(writer, packet, 1, &written, NULL));
    assert(TdHidReadInputReport((int*)reader, buffer, sizeof(buffer), 500) == TDHID_ERR_IO);
    CloseHandle(writer);
    assert(TdHidReadInputReport((int*)reader, buffer, sizeof(buffer), 500) == TDHID_ERR_IO);
    CloseHandle(reader);
    puts("PASS: Windows timeout, cancellation, infinite wait, short read and disconnect");
    return 0;
}
