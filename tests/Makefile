CC = gcc
CFLAGS = -Wall -Wextra -g

.PHONY: test clean
test: test_receive test_libusb_read
	./test_receive
	./test_libusb_read

test_receive: test_receive.c ../tddevice.c ../tddevice.h ../tdhid.h ../tdtimer.h ../td-usb.h $(wildcard ../devices/*.c)
	$(CC) $(CFLAGS) test_receive.c ../tddevice.c ../devices/*.c -o $@ -lm

test_libusb_read: test_libusb_read.c ../linux/tdhid-libusb.c ../tdhid.h
	$(CC) $(CFLAGS) test_libusb_read.c ../linux/tdhid-libusb.c -o $@ -lusb -Wl,--wrap=usb_interrupt_read

clean:
	rm -f test_receive test_libusb_read
