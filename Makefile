CC := gcc
CFLAGS := -Wall

td-usb: td-usb.c device_types.c tdhid-libusb.c tdtimer-posix.c tddevice.c ./devices/*.c
	$(CC) $(CFLAGS) td-usb.c device_types.c tddevice.c tdhid-libusb.c tdtimer-posix.c ./devices/*.c -o td-usb -lusb -lrt -lm

clean:
	rm td-usb
	rm -f *.o
