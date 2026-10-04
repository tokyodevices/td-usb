# Receive regression tests

The tests do not access USB hardware. Response tests cover 8-byte and 64-byte reports; event listeners include TDFA60228.

On Linux, run `make -C tests test`. This tests response deadlines, GET retries,
legacy ACKs, sensor replies, all event listeners, and the libusb read adapter.
USB reports and elapsed time are simulated for the response tests.
TDPC2401 tests cover register access, unsigned 32-bit boundaries, invalid inputs,
read-only registers, write order, output formats and initialization.

On Windows, from an x64 Visual Studio developer command prompt at the repository
root (without `NDEBUG`):

```
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_receive.c tddevice.c devices\*.c /Fe:tests\test_receive.exe
tests\test_receive.exe
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_windows_read.c windows\tdhid-windows.c windows\tdtimer-windows.c /Fe:tests\test_windows_read.exe /link hid.lib setupapi.lib
tests\test_windows_read.exe
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_tdpc2401.c devices\tdpc2401.c /Fe:tests\test_tdpc2401.exe
tests\test_tdpc2401.exe
```

The Windows adapter test uses a local overlapped pipe to exercise cancellation,
subsequent reads, a delayed event beyond three seconds, short reads, and disconnect.

Response waits use a three-second deadline per request, including unrelated Input
reports. Register GET retains up to three attempts (at most nine seconds of
response waiting); SET and SAVE are not retried. User-facing `listen` passes a
zero timeout and waits indefinitely on both platforms.
