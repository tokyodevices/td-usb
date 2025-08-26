CC := gcc
CFLAGS := -Wall

# Platform detection
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    # macOS specific settings
    TIMER_SRC := ./mac/tdtimer-mac.c
    INCLUDES := -I/opt/homebrew/include -I/opt/homebrew/include/libusb-compat-0.1
    LDFLAGS := -L/opt/homebrew/lib
    LIBS := -lusb -framework IOKit -framework CoreFoundation
else
    # Linux specific settings
    TIMER_SRC := ./linux/tdtimer-posix.c
    INCLUDES := 
    LDFLAGS := 
    LIBS := -lusb -lrt -lm
endif

td-usb: td-usb.c device_types.c ./linux/tdhid-libusb.c $(TIMER_SRC) tddevice.c ./devices/*.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) td-usb.c device_types.c tddevice.c ./linux/tdhid-libusb.c $(TIMER_SRC) ./devices/*.c -o td-usb $(LIBS)

clean:
	rm -f td-usb
	rm -f *.o
