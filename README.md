# TD-USB

CLI(Command Line Interface) for USB-based products of Tokyo Devices, Inc.

## Usage and Integration

Visit [TD-USB Developer Guide](https://tokyodevices.github.io/td-usb-docs/). (Japanese only)


## Build & Install

### Dependency

#### For windows

`Setupapi.lib` and `Hid.lib` are required and those must be available on your Visual Studio environment. 
If you do not have them, try to search Windows Driver Kit or Windows SDK. 

#### For linux

TD-USB depends on `libusb-dev` package.
You should install it before the compile.
ex.) `apt install libusb-dev` for Ubuntu.

### Compile

#### For windows

Open `td-usb.sln` by Visual Studio and build the project.


#### For linux

1. Clone the source to your working directory. 


    % git clone https://github.com/tokyodevices/td-usb


2. Run make.


    % cd td-usb
    % make


3. Run with no-option shows some software information.


    % ./td-usb
    td-usb version 0.1
    Copyright (C) 2020-2021 Tokyo Devices, Inc. (tokyodevices.jp)
    Usage: td-usb model_name[:serial] operation [options]
    Visit https://github.com/tokyodevices/td-usb/ for details


### Device permission

USB devices that are connected to Linux platform always be under control of `udev` system.
Thus in most case it can be accessed only by root user by default. 

To access the device by normal user, `udev` needs some snippet to apply the permission. 

For Ubuntu or Debian, try to create `/etc/udev/rules.d/99-usb-tokyodevices.rules` which includes following line:

    SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05df", MODE="0666"

Note that `16c0` and `05df` should be replaced to VID/PDF of the device you need to use. 



## License

This software is distributed in the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).
