# Receive regression tests

The tests do not access USB hardware. Response tests cover 8-byte and 64-byte reports; event listeners include TDFA60228.

On Linux, run `make -C tests test`. This tests response deadlines, GET retries,
legacy ACKs, sensor replies, all event listeners, and the libusb read adapter.
USB reports and elapsed time are simulated for the response tests.
Linux timer tests check initial and repeating intervals, including 3/5 seconds,
the former 32-bit nanosecond overflow boundaries, and INT_MAX milliseconds.
Timer requests are intercepted so these checks do not wait for each interval.
TDPC2401 tests cover register access, unsigned 32-bit boundaries, invalid inputs,
read-only registers, write order, output formats and initialization.
TDFA60228 tests cover all eight channels, register addresses and limits, per-device
calibration and invalidation after SET, raw/current output, malformed DUMPs,
channel selection, and skipping whole scans without losing channel alignment.
They also check the explicit `0xFFFF` measurement-unavailable error in GET/DUMP,
in both raw and current formats, with no partial output and validation before skip.

On Windows, from an x64 Visual Studio developer command prompt at the repository
root (without `NDEBUG`):

```
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_receive.c tddevice.c devices\*.c /Fe:tests\test_receive.exe
tests\test_receive.exe
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_windows_read.c windows\tdhid-windows.c windows\tdtimer-windows.c /Fe:tests\test_windows_read.exe /link hid.lib setupapi.lib
tests\test_windows_read.exe
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_windows_enumeration.c /Fe:tests\test_windows_enumeration.exe /link hid.lib setupapi.lib
tests\test_windows_enumeration.exe
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_list_cli.c /Fe:tests\test_list_cli.exe
tests\test_list_cli.exe stable list
tests\test_list_cli.exe empty list
tests\test_list_cli.exe shrink list
tests\test_list_cli.exe grow list
tests\test_list_cli.exe allocation-failure list
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_tdpc2401.c devices\tdpc2401.c /Fe:tests\test_tdpc2401.exe
tests\test_tdpc2401.exe
cl /nologo /D_CRT_SECURE_NO_WARNINGS tests\test_tdfa60228.c devices\tdfa60228.c /Fe:tests\test_tdfa60228.exe
tests\test_tdfa60228.exe
```

The enumeration test mocks device discovery and uses local event handles. It
checks string conversion, full-length USB strings, exact and insufficient list
buffers, hotplug growth, and cleanup after discovery/allocation/attribute errors.
The CLI fixture runs the real argument parsing and list command with simulated
devices. `stable` and `shrink` print `1111`; `empty` prints a blank line, all with
exit code 0. `grow` and `allocation-failure` must exit with code 11, produce no
stdout, and report the corresponding error on stderr. No USB hardware is used.

The Windows adapter test uses a local overlapped pipe to exercise cancellation,
subsequent reads, a delayed event beyond three seconds, short reads, and disconnect.

Response waits use a three-second deadline per request, including unrelated Input
reports. Register GET retains up to three attempts (at most nine seconds of
response waiting); SET and SAVE are not retried. User-facing `listen` passes a
zero timeout and waits indefinitely on both platforms.
