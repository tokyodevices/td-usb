
[Japanese(Nihongo)](README_ja.md)

# TD-USB

CLI(Command Line Interface) for USB-based products of Tokyo Devices, Inc.


## Target platform

- Microsoft Windows 7 or later (64bit)
- Linux


## Product models

|Model number|Name|`model_name` string|
|-------|-----|---------------|
|IWT120-USB|[Generic USB LED/Buzzer Module](https://en.tokyodevices.com/items/201)|`iwt120`|
|IWT1320-USB|[General Purpose USB Slider Input Device with integrated LED and Buzzer](https://en.tokyodevices.com/items/271)|`iwt1320`|
|IWS660-CS|[Generic USB Illuminance Meter](https://en.tokyodevices.com/items/228)|`iws660`|
|IWS73X-CS|[Generic USB CO2 Meter](https://en.tokyodevices.com/items/205)|`iws73x`|
|IWT303-1C|[USB SPDT Relay Controller 1CH](https://en.tokyodevices.com/items/148)|`iwt303`|
|IWT303-3C|[USB SPDT Relay Controller 3CH](https://en.tokyodevices.com/items/149)|`iwt303`|
|IWT313-USB|[USB SPDT Relay Controller 8CH](https://en.tokyodevices.com/items/149)|`iwt313`|
|TDFA30608|[8CH USB Digital Input Module with Optical Isolation](https://en.tokyodevices.com/items/284)|`tdfa30608`|
|TDFA30604|4CH USB Digital Input Module with Optical Isolation|`tdfa30604`|
|TDFA50507|[7CH USB Digital Output Module with Optical Isolation and Current Sink](https://en.tokyodevices.com/items/308)|`tdfa50507`|
|TDFA50607|[USB Digital Output Board, 7 Channels, 5/12/24V Output, Current Source Output, Bus-powered](https://en.tokyodevices.com/items/345)|`tdfa50607`|
|TDFA60220|[USB AD Converter For 4-20mA Analog Current Signal with Galvanic Isolation](https://en.tokyodevices.com/items/311)|`tdfa60220`|
|TDFA60250|[USB AD Converter For 0-5V/1-5V Analog Signal with Galvanic Isolation](https://en.tokyodevices.com/items/314)|`tdfa60250`|
|TDFA6032A|[USB Analog Output, 0-20mA/4-20mA Current Signal, Galvanic Isolation](https://en.tokyodevices.com/items/315)|`tdfa6032`|
|TDFA60350|[USB to 0-5V/1-5V, Analog Signal Output, Galvanic Isolation, Bus-Powered](https://en.tokyodevices.com/items/323)|`tdfa60350`|
|TDPC0201|["Reset Master" - a USB Watchdog](https://en.tokyodevices.com/items/288)|`tdpc0201`|
|TDPC0205|["Clock Dongle" - a USB Real-Time Clock for Current Time Sync., Battery Backup](https://en.tokyodevices.com/items/319)|`tdpc0205`|
|TDSN0700-UA|[USB General Purpose UV Sensor, UVA 295-490nm, Bus-Powered](https://en.tokyodevices.com/items/321)|`tdsn0700`|
|TDSN0700-UB|[USB General Purpose UV Sensor, UVB 240-320nm, Bus-Powered](https://en.tokyodevices.com/items/322)|`tdsn0700`|
|TDSN5200|[General-purpose, USB ToF Laser Distance Meter, Up to 6m, Bus-Powered](https://en.tokyodevices.com/items/326)|`tdsn5200`|
|TDSN604A8|[Genral-purpose Precision USB Current Sensor 20 Bit 0.8A Galvanic Isolation Bus-Powered](https://en.tokyodevices.com/items/332)|`tdsn604xx`|
|TDSN60408|[Genral-purpose Precision USB Current Sensor 20 Bit 8A Galvanic Isolation Bus-Powered](https://en.tokyodevices.com/items/331)|`tdsn604xx`|
|TDSN60420|[Genral-purpose Precision USB Current Sensor 20 Bit 20A Galvanic Isolation Bus-Powered](https://en.tokyodevices.com/items/330)|`tdsn604xx`|
|TDSN7200|[General-Purpose High-Precision USB Temperature, Humidity, and Pressure Sensor, -40 to 125ÅãC, 0 to 100%RH, 260 to 1,260hPa, Bus Powered](https://en.tokyodevices.com/items/327)|`tdsn7200`|
|TDSN7300|[Air Analyzer 2: Generic USB CO2 Meter](https://en.tokyodevices.com/items/356)|`tdsn7300`|
|TDSN7360|[General-purpose high-precision USB accelerometer, 3-axis, low-noise, Å}2.5g, 440Hz, bus-powered](https://en.tokyodevices.com/items/335)|`tdsn7360`|
|TDSN7400|[General purpose USB thermocouple temperature sensor K type -40 to 1200Åé galvanic isolation Bus-Powered](https://en.tokyodevices.com/items/333)|`tdsn7400`|
|TDSN7502|[General purpose, Load cell to USB interface, Weight/Pressure sensor, 24-bit, Bus-power](https://en.tokyodevices.com/items/344)|`tdsn7502`|
|TDFA1104|[General purpose USB 7 segment LED display, 4 digits, 0.8 inch height, red](https://en.tokyodevices.com/items/350)|`tdfa1104`|

## Build instructions

Windows users are recommended to download and run the pre-built executable from
 [the Release page]((https://github.com/tokyodevices/td-usb/releases)).
For Linux users or Windows users who prefer to build it themselves, please follow the steps below to compile.

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
Please exclude the source code contained in the `/linux` folder from the build.

#### On Linux

Clone this repository to working directory. 


    % git clone https://github.com/tokyodevices/td-usb


Run make.


    % cd td-usb
    % make


Run `td-usb` with no-option shows version information.


    % ./td-usb
    td-usb version 0.2.6
    Copyright (C) 2020-2022 Tokyo Devices, Inc. (tokyodevices.jp)
    Usage: td-usb model_name[:serial] operation [options]
    Visit https://github.com/tokyodevices/td-usb/ for details


**Setting device permission**

USB devices that are connected to Linux platform firstly be under control of `udev` system.
Thus in most case it can only be accessed by root user. 

To add permission for normal user, `udev` needs some snippet. 
For Ubuntu or Debian, try to create `/etc/udev/rules.d/99-usb-tokyodevices.rules` which includes following line:

    SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="05df", MODE="0666"

Note that `16c0` and `05df` should be replaced to VID/PID of the device which you need to use. 


## Usage

Available operation and option differ depending on the product model.


### Specifying product model

    td-usb (model_name) (operation) [options]

- `model_name` is fixed string that specifies product model. ex) `tdfa30608`


### Reading a value from the device

    td-usb (model_name) get [--format=(format)] [--loop=(interval)] [options]

- With `--loop` it will be repeated by `interval` milli-seconds.
- `[options]` is defiend by the product model.

### Writing a value to the device

    td-usb (model_name) set [(name)=(value)] [--loop]

 `(name)=(value)` pair is defiend by the product model.

With `--loop` option, TD-USB read repeatedly `(name)=(value)` pair from the standard input repeatedly.

     td-usb model_name[:serial] set --loop


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



## Exit code and error definition

|Exit Code|Name                        |Description                                 |
|---------|----------------------------|--------------------------------------------|
|0        |NO_ERROR                    |The process terminated successfully.|
|2        |UNKNOWN_DEVICE              |`model_name` is invalid.  |
|3        |UNKNOWN_OPERATION           |`operation` is invalid.    |
|4        |OPERATION_NOT_SUPPORTED     |Specified `operation` is not supproted for the device. |
|6        |INVALID_OPTION              |Invalid option format. |
|11       |DEVICE_OPEN_ERROR           |Failed to open the device. |
|12       |DEVICE_IO_ERROR             |Failed to communicate to the device. |
|13       |INVALID_FORMAT              |Specified `--format` is not available for the device. |
|14       |INVALID_RANGE			   |The given value is out of range. |


## License

TD-USB is released under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).
