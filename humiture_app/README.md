## Overview

This project demonstrates how to simultaneously read temperature and humidity from two sensors: SHTC3 and AM2320 and sent this data to ESP RainMaker app. Duration between measurements is set to one minute. Communication between them and  the microcontroller is established through I2C bus.

## How to use code

First you need to download repositories from https://github.com/espressif/esp-idf and https://github.com/espressif/esp-rainmaker. Place humiture_app directory in           esp-rainmaker/examples.

### Configure the project

Open the project configuration menu (`idf.py menuconfig`). Then go into `Project Configuration` menu.

- In the `I2C Master` submenu, you can set the pin number of SDA/SCL according to your board. GPIO3 and GPIO2 are set as default for SDA and SCL.
- In the `SHTC3 Sensor` submenu, you can choose one of 4 operation modes for this sensor. The differences between modes are whether clock stretching is enabled or disabled and whether temperature or humidity is read first.

### Hardware Required

To run this code, you should have one ESP development board or core board (I used ESP32-C3-DevKitM-1), one SHTC3 sensor and one AM2320 sensor. No external pull-up resistors are needed as there are a pair of pull-up resistors soldered on the SHTC3's board.

I have set GPIO2 as SCL and GPIO3 as SDA. So connect AM2320's SCL and SDA pins to GPIO2 and GPIO3 pins of the microcontroller and SHTC3's SCL and SDA to AM2320's ones. Also, connect VCC and GND of sensors to 5V and GND on the microcontroller.

### Build and Flash

Enter `idf.py -p PORT flash monitor` to build, flash and monitor the project.

Install ESP RainMaker app on your phone. Scan the QR code (while Bluetooth is turned on) to add the device to the app.
