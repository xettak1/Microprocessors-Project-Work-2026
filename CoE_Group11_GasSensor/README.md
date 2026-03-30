# KNUST Gas Leakage Detection System

## Project Description
A real-time gas leakage detection and alert system built on the ESP32
microcontroller. The system monitors air quality continuously, triggers
local alarms through a buzzer and LEDs, displays live readings on an
LCD screen, and sends instant alerts to a Telegram group chat.

## Components
- ESP32 Microcontroller
- MQ-2 Gas Sensor
- 16x2 I2C LCD Display
- Passive Buzzer
- Red LED and Green LED
- Jumper Wires and Breadboard

## PPM Thresholds
- 0 to 950 PPM        : SAFE
- 951 to 1150 PPM     : CAUTION
- 1151 PPM and above  : DANGER

## Telegram Commands
Everyone in the group:
/status, /history, /test_alarm, /restart, /help

Admins only:
/alarm_off, /alarm_on, /reset_alarm, /shutdown, /reset_system

## Implementation
Built on physical hardware using an ESP32 development board.
Tested and demonstrated live.

## Folder Structure
- code/           Arduino source code
- schematics/     Circuit schematic diagram
- hardware/       Photos of the physical build and live demo
- documentation/  Full project report