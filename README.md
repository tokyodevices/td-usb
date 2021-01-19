# TD-USB

CLI(Command Line Interface) for USB-based products of Tokyo Devices, Inc.

[https://tokyodevices.com/](https://tokyodevices.com/)

## Usage

See [TD-USB Developer Guide](https://tokyodevices.github.io/td-usb-docs/). (Japanese only)


## Build instructions

### Dependency

#### On Windows

`Setupapi.lib` and `Hid.lib` are required and those must be available on your Visual Studio environment. 
If you do not have them, try to search Windows Driver Kit or Windows SDK. 

#### On Linux

TD-USB is depend on `libusb-dev` package.
You should install it before compile.
ex.) `apt install libusb-dev` for Ubuntu/Debian.

### Compile

#### On Windows

Open `td-usb.sln` by Visual Studio and build the project.
Note that you may add library and include path for `Setupapi.lib` and `Hid.lib`.

#### On Linux

Clone this repository to working directory. 


    % git clone https://github.com/tokyodevices/td-usb


Run make.


    % cd td-usb
    % make


Run `td-usb` with no-option shows version information.


    % ./td-usb
    td-usb version 0.1
    Copyright (C) 2020-2021 Tokyo Devices, Inc. (tokyodevices.jp)
    Usage: td-usb model_name[:serial] operation [options]
    Visit https://github.com/tokyodevices/td-usb/ for details


### Device permission

USB devices that are connected to Linux platform firstly be under control of `udev` system.
Thus in most case it can only be accessed by root user. 

To add permission for normal user, `udev` needs some snippet. 
For Ubuntu or Debian, try to create `/etc/udev/rules.d/99-usb-tokyodevices.rules` which includes following line:

    SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05df", MODE="0666"

Note that `16c0` and `05df` should be replaced to VID/PDF of the device you need to use. 

## License

TD-USB is released under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).
