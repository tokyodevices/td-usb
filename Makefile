CC := gcc
CFLAGS := -Wall

td-usb: td-usb.c device_types.c tdhid-libusb.c tdtimer-posix.c ./devices/*.c 
	$(CC) $(CFLAGS) td-usb.c device_types.c tdhid-libusb.c tdtimer-posix.c ./devices/*.c -o td-usb -lusb -lrt

clean:
	rm td-usb
	rm -f *.o
