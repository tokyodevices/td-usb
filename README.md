# TD-USB

CLI(Command Line Interface) for USB-based products of Tokyo Devices, Inc.

[https://tokyodevices.com/](https://tokyodevices.com/)

## Target platform

- Microsoft Windows 7 or later 32/64bit  
  (Depricated support on Microsoft Windows XP SP3)
- Linux

## Build instructions

### Dependency

#### On Windows

- Microsoft Visual Studio is required to build.
- `Setupapi.lib` and `Hid.lib` must be available on library path. 
If you do not have these library, try to search Windows Driver Kit or Windows SDK. 

#### On Linux

- Git, GCC compiler and build toolkit
- TD-USB is depend on `libusb-dev` package. You should install it before compile.  
   ex.) `apt install libusb-dev` for Ubuntu/Debian.

### Compile

#### On Windows

Open `td-usb.sln` by Microsoft Visual Studio and build the project.
Note that you may add library and include path for `Setupapi.lib` and `Hid.lib`.

#### On Linux

Clone this repository to working directory. 


    % git clone https://github.com/tokyodevices/td-usb


Run make.


    % cd td-usb
    % make


Run `td-usb` with no-option shows version information.


    % ./td-usb
    td-usb version 0.2
    Copyright (C) 2020-2021 Tokyo Devices, Inc. (tokyodevices.jp)
    Usage: td-usb model_name[:serial] operation [options]
    Visit https://github.com/tokyodevices/td-usb/ for details


### Setting device permission

USB devices that are connected to Linux platform firstly be under control of `udev` system.
Thus in most case it can only be accessed by root user. 

To add permission for normal user, `udev` needs some snippet. 
For Ubuntu or Debian, try to create `/etc/udev/rules.d/99-usb-tokyodevices.rules` which includes following line:

    SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05df", MODE="0666"

Note that `16c0` and `05df` should be replaced to VID/PID of the device which you need to use. 


## Usage

Available operation and option differ depending on the product model.

See [TD-USB Developer Guide](https://tokyodevices.github.io/td-usb-docs/). (Japanese only)


### Specifying product model

    td-usb (model_name) (operation) [options]

- `model_name` is fixed string that specifies product model. ex) `tdfa30608`


### Reading a value from the device

    td-usb (model_name) read [--format=(format)] [--loop=(interval)] [options]

- With `--loop` it will be repeated by `interval` milli-seconds.
- `[options]` is defiend by the product model.

### Writing a value to the device

    td-usb (model_name) write [options]

- `[options]` is defiend by the product model.


### Listening to an event from the device

    td-usb (model_name) listen [--loop] [options]

- The process waits for an event and, when it is received, prints the event parameters to standard out.
- With `--loop` option the process will repeat continuously, otherwise terminate.
- `[options]` is defiend by the product model.


### Save current value of device registers

    td-usb (model_name) save

- The values will be loaded on next power-on sequence.


### Remove all firmware to switch DFU mode

    td-usb (model_name) destroy

- After this operation the device will be unavailable until new firmware is written.




## Usage

See [TD-USB Developer Guide](https://tokyodevices.github.io/td-usb-docs/). (Japanese only)


### Read operation

    td-usb (model_name) read [--format=(format)] [--loop=(interval)] [options]

- Reading a value from specified device.
- `[options]` is defiend by each product model.
- With `--loop` it will be repeated by `interval` milli-seconds.

### Write operation

    td-usb (model_name) write [args]

- Writing a value to the specified device.
- `[args]` is defiend by each product model.


### Listen operation

    td-usb (model_name) listen [--loop] [options]

- Listening to an event from the specified device.
- The process waits for an event and, when it is received, prints the event parameters to standard out.
- With `--loop` option the process will repeat continuously, otherwise terminate.
- `[options]` is defiend by each product model.



## License

TD-USB is released under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).
