\# STM32 SmartDesk FreeRTOS



A FreeRTOS-based STM32F411 smart desk monitoring system with OLED multi-page UI, ESP8266 WiFi networking, SNTP time synchronization, Seniverse weather API, sensor data display, alarm mode and actuator control.



\## Features



\- STM32F411RET6 + FreeRTOS dual-task architecture

\- OLED multi-page UI over I2C

\- Button short press page switching and long press mode switching

\- ESP8266 AT command communication over UART

\- WiFi connection and status detection

\- SNTP network time synchronization

\- Seniverse real-time weather acquisition

\- Periodic weather update task

\- Weather updating does not block OLED UI or button input

\- I2C/OLED recovery mechanism

\- Sensor data layer and actuator alarm control



\## System Architecture



\### appCoreTask



Responsible for:



\- OLED UI refresh

\- Button scanning

\- Software clock update

\- Sensor data update

\- Actuator alarm update



\### weatherTask



Responsible for:



\- ESP8266 AT command test

\- WiFi connection and status check

\- SNTP time synchronization

\- Seniverse weather HTTP request

\- Periodic weather update



\## Hardware



\- NUCLEO-F411RE / STM32F411RET6

\- ESP8266 WiFi module

\- 0.96-inch SSD1306 OLED

\- Light sensor

\- MQ gas sensor module

\- Buzzer / onboard LED actuator



\## Current Status



\- OLED UI: OK

\- Button page switching: OK

\- WiFi: OK

\- SNTP: OK

\- Weather API: OK

\- Periodic weather update: OK

\- 30-minute stability test: passed



\## Private Configuration



Create your own private weather configuration file:



```c

User/App/app\_weather\_config.h
This file is ignored by Git. Do not commit API keys.

Use the example file as reference:

User/App/app_weather_config.h.example
Development Environment
STM32CubeMX
Keil MDK
STM32 HAL
FreeRTOS

