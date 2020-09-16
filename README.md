# IoT Binders
Using micropocessors and microcontrollers with an accelerometer on your binders and backpack to keep you organized.
It uses an array of ESP32 nodes with accelerometers (one per binder) and a raspberry pi zero as a gateway also with an accelerometer.

## How it Works
The purpose of the system is to notify the user if they are forgetting something in their bag. When a binder is not in the bag, its motion will be different than the backpack.

## Hardware
### ESP32 and MPU6050
The ESP32 is used for the binders to commmunicate via WIFI using MQTT protocol. They tranmit their accelerometer data to compare with the raspberry pi.
The MPU6050 communicates via I2C to the ESP32. The systems only uses one axis of acceleration to determine whether the binder is insider the bag.

### Raspberry Pi Zero W
The Raspberry pi acts as an IoT gateway and provides the ESP32 with a 2.4ghz LAN. NodeRED runs on the Raspberry Pi providing a framework for the Raspberry Pi to handle MQTT messages from the ESP32. The accelerometer data comparison is done with on a C++ program running on the Pi which uses a parrallel thread to retieve the data from the onboard accelerometer (also an MPU6050). 

## Project Status
On hold as COVID-19 has delayed the international postage services. So I'm missing several components to make this project full functioning.
