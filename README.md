# Vehicle Sensoring and Tracking system
 Final project of the master's course "Real Time Embedded Systems Design".
 
 ## Description
This systems consists on sending GPS and IMU (acceleration and angular velocity) data from a vehicle to a cloud server.
 
It uses a STM32G474RE MCU running FreeRTOS, alongside with a SIM808 Radio (GPRS/GSM radio + GPS) and a MPU-6050 IMU. It also has a FIR filter used to filter the IMU data.

System data is sent to the cloud using TCP connection and the HTTP protocol. The API used is the ThingSpeak.com free API tool.

## System diagrams
Below you can see some system diagrams created on the development process.

![Block diagram](https://imgur.com/YsqunT8)
System block diagram (in Portuguese).

![Electric diagram](https://imgur.com/pMMgvs7)
System electric diagram.

![RTOS diagram](https://imgur.com/LcaBK3j)
System RTOS diagram (in Portuguese).
