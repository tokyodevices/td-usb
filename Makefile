CC := gcc
CFLAGS := -Wall

td-usb: td-usb.c device_types.c ./linux/tdhid-libusb.c ./linux/tdtimer-posix.c tddevice.c ./devices/*.c
	$(CC) $(CFLAGS) td-usb.c device_types.c tddevice.c ./linux/tdhid-libusb.c ./linux/tdtimer-posix.c ./devices/*.c -o td-usb -lusb -lrt -lm

clean:
	rm td-usb
	rm -f *.o
