# STM32 Environment Sensing and Actuator Control Terminal

A FreeRTOS-based STM32F411 device terminal for environment sensing, OLED status display, WiFi communication, weather information display, alarm logic, and actuator control.

This project is the **device layer** of a larger EdgeAIoT system. It provides sensor data, device status, and actuator control capability for future integration with a Linux-STM32 IoT gateway and an Orange Pi edge AI control system.

## Project Position

```text
STM32 Device Layer
        ↓
Linux-STM32 IoT Edge Gateway
        ↓
RAG-based Knowledge Service
        ↓
Orange Pi + STM32 Edge AIoT Control System
```

This repository focuses on the STM32 device terminal, including:

* Sensor data acquisition
* OLED multi-page UI
* WiFi communication
* SNTP time synchronization
* Weather API acquisition
* Alarm mode and actuator control
* FreeRTOS task scheduling

## Current Version

### V1.0 - NUCLEO-F411RE Functional Prototype

The current version is based on **NUCLEO-F411RE / STM32F411RET6** and implements a functional prototype of the device terminal.

Implemented features:

* STM32F411RET6 + FreeRTOS dual-task architecture
* OLED multi-page UI over I2C
* Button short-press page switching and long-press mode switching
* ESP8266 AT command communication over UART
* WiFi connection and status detection
* SNTP network time synchronization
* Seniverse real-time weather acquisition
* Periodic weather update task
* Weather updating without blocking OLED UI or button input
* I2C / OLED recovery mechanism
* Sensor data layer
* Actuator alarm control interface
* 30-minute stability test passed

## System Architecture

```text
+------------------------------------------------+
|                  appCoreTask                   |
|  OLED UI / Button / Clock / Sensor / Actuator  |
+------------------------------------------------+
                         |
                         |
+------------------------------------------------+
|                  Shared Data Layer             |
|  Mode / Sensor Data / Time / Weather / Alarm   |
+------------------------------------------------+
                         |
                         |
+------------------------------------------------+
|                  weatherTask                   |
|  ESP8266 / WiFi / SNTP / Weather HTTP Request  |
+------------------------------------------------+
                         |
                         |
+------------------------------------------------+
|                  Hardware Layer                |
|  GPIO / I2C / UART / ADC / Timer / FreeRTOS    |
+------------------------------------------------+
```

## Task Design

### appCoreTask

Responsible for real-time device-side logic:

* OLED UI refresh
* Button scanning
* Software clock update
* Sensor data update
* Actuator alarm update

### weatherTask

Responsible for network-related logic:

* ESP8266 AT command test
* WiFi connection and status check
* SNTP time synchronization
* Seniverse weather HTTP request
* Periodic weather update

The network task is separated from the core UI and device task to reduce blocking impact on OLED refresh and button input.

## Main Modules

| Module   | Description                                                            |
| -------- | ---------------------------------------------------------------------- |
| OLED UI  | Displays home page, device state, sensor data, and weather information |
| Button   | Supports page switching and mode switching                             |
| Sensor   | Updates environment sensing data                                       |
| Actuator | Handles alarm indication and reserved actuator control                 |
| ESP8266  | Handles WiFi communication through AT commands                         |
| Weather  | Gets time and weather information through network services             |
| FreeRTOS | Separates device-side logic and network-side logic                     |

## Hardware

Current prototype hardware:

* NUCLEO-F411RE / STM32F411RET6
* ESP8266 WiFi module
* 0.96-inch SSD1306 OLED
* Light sensor
* MQ gas sensor module
* Buzzer / onboard LED actuator interface

## Current Status

| Function                              | Status |
| ------------------------------------- | ------ |
| OLED UI                               | OK     |
| Button page switching                 | OK     |
| WiFi connection                       | OK     |
| SNTP time synchronization             | OK     |
| Weather API acquisition               | OK     |
| Periodic weather update               | OK     |
| UI non-blocking during weather update | OK     |
| I2C / OLED recovery                   | OK     |
| Sensor data layer                     | OK     |
| Actuator alarm control                | OK     |
| 30-minute stability test              | Passed |

## Private Configuration

The weather API key should not be committed to Git.

Create your own private configuration file:

```text
User/App/app_weather_config.h
```

This file is ignored by Git.

Use the example file as reference:

```text
User/App/app_weather_config.h.example
```

## Development Environment

* STM32CubeMX
* Keil MDK
* STM32 HAL
* FreeRTOS
* Git / GitHub

## Refactor Direction

The next stage of this project is a modular rebuild on an **STM32F411 core board**.

The goal of the refactor is not only board-level migration, but also rebuilding the project from scratch with clearer module boundaries and deeper understanding of each module.

The V2 refactor will focus on:

* Rebuilding the project from a new CubeMX configuration
* Separating board-level configuration from application logic
* Re-implementing OLED UI, button input, sensor acquisition, actuator control, ESP8266 communication, SNTP/weather service, and FreeRTOS task scheduling step by step
* Preparing a communication protocol for future Linux-STM32 IoT gateway integration
* Making the project easier to understand, maintain, and explain in technical interviews

Target V2 module structure:

```text
Board Support Layer
        ↓
Application Data Layer
        ↓
UI / Key / Sensor / Actuator Modules
        ↓
ESP8266 / Weather / Communication Modules
        ↓
FreeRTOS Task Scheduling
```

## Version History

### V1.0 - NUCLEO-F411RE Functional Prototype

Implemented the first functional prototype based on NUCLEO-F411RE, including OLED UI, button input, ESP8266 WiFi communication, SNTP time synchronization, weather display, sensor data layer, alarm logic, and FreeRTOS task scheduling.

### V2 - STM32F411 Core Board Modular Refactor

The project will be rebuilt on an STM32F411 core board with clearer module boundaries, board-level abstraction, and step-by-step reimplementation of core modules.

## Goal

The goal of this project is to build a reliable STM32 device terminal that can serve as the environment sensing and actuator control node of an EdgeAIoT system.
