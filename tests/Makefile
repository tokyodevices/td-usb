CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: test clean
test: test_common_cli test_receive test_libusb_read test_libusb_enumeration test_list_cli test_tdpc2401 test_tdpw6620 test_tdfa60228 test_tdtimer_posix
	./test_common_cli
	./test_receive
	./test_libusb_read
	./test_libusb_enumeration
	./test_list_cli stable list
	./test_list_cli empty list
	./test_list_cli shrink list
	@for scenario in grow allocation-failure discovery-size-failure discovery-fill-failure; do \
		./test_list_cli $$scenario list; status=$$?; \
		test $$status -eq 11 || exit 1; \
	done
	./test_tdpc2401
	./test_tdpw6620
	./test_tdfa60228
	./test_tdtimer_posix

test_common_cli: test_common_cli.c ../td-usb.c ../device_types.c ../td-usb.h ../tdhid.h ../tdtimer.h ../tddevice.h ../tdthread.h
	$(CC) $(CFLAGS) test_common_cli.c -o $@

test_receive: test_receive.c ../tddevice.c ../tddevice.h ../tdhid.h ../tdtimer.h ../td-usb.h $(wildcard ../devices/*.c)
	$(CC) $(CFLAGS) test_receive.c ../tddevice.c ../devices/*.c -o $@ -lm

test_libusb_read: test_libusb_read.c ../linux/tdhid-libusb.c ../tdhid.h
	$(CC) $(CFLAGS) test_libusb_read.c ../linux/tdhid-libusb.c -o $@ -lusb -Wl,--wrap=usb_interrupt_read -Wl,--wrap=usb_control_msg

test_libusb_enumeration: test_libusb_enumeration.c ../linux/tdhid-libusb.c ../tdhid.h
	$(CC) $(CFLAGS) test_libusb_enumeration.c ../linux/tdhid-libusb.c -o $@

test_list_cli: test_list_cli.c ../td-usb.c ../td-usb.h ../tdhid.h ../tddevice.h ../tdtimer.h ../tdthread.h
	$(CC) $(CFLAGS) test_list_cli.c -o $@

test_tdpc2401: test_tdpc2401.c ../devices/tdpc2401.c ../tddevice.h ../td-usb.h
	$(CC) $(CFLAGS) test_tdpc2401.c ../devices/tdpc2401.c -o $@

test_tdpw6620: test_tdpw6620.c ../devices/tdpw6620.c ../tddevice.h ../td-usb.h
	$(CC) $(CFLAGS) test_tdpw6620.c ../devices/tdpw6620.c -o $@

test_tdfa60228: test_tdfa60228.c ../devices/tdfa60228.c ../tddevice.h ../tdhid.h ../td-usb.h
	$(CC) $(CFLAGS) test_tdfa60228.c ../devices/tdfa60228.c -o $@

test_tdtimer_posix: test_tdtimer_posix.c ../linux/tdtimer-posix.c ../tdtimer.h ../td-usb.h
	$(CC) $(CFLAGS) test_tdtimer_posix.c ../linux/tdtimer-posix.c -o $@ -lrt -Wl,--wrap=timer_settime

clean:
	rm -f test_common_cli test_receive test_libusb_read test_libusb_enumeration test_list_cli test_tdpc2401 test_tdpw6620 test_tdfa60228 test_tdtimer_posix
