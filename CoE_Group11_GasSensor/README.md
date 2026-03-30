# Real-Time Gas Leakage Detection and Alert System (ESP32)

## Project Overview
This project is an IoT-based safety solution designed to monitor air quality and detect gas leaks in real-time. Built on the ESP32 microcontroller, the system detects combustible gases using an MQ-2 sensor. It provides immediate local feedback via an LCD and alarms, while leveraging the Telegram Bot API to send instant remote alerts to a group chat.

### Key Features
* Continuous Monitoring: Real-time PPM (Parts Per Million) tracking.
* Triple Alert System:
    1. Visual: 16x2 I2C LCD Display and Status LEDs (Green/Red).
    2. Audible: Passive Buzzer for critical danger levels.
    3. Remote: Instant Telegram notifications.
* Interactive Control: A custom Telegram bot interface allowing users and admins to interact with the hardware remotely.
* Admin Security: Tiered command access ensuring only authorized admins can reset or shutdown the system.

---

## Hardware Components
| Component | Purpose |
| :--- | :--- |
| ESP32 DevKit | Main processing unit with Wi-Fi connectivity |
| MQ-2 Sensor | Detects LPG, Propane, Hydrogen, and Smoke |
| 16x2 I2C LCD | Displays live PPM readings and system status |
| Passive Buzzer | High-frequency audible alarm |
| LEDs (Red/Green) | Visual indicators for Safe vs Danger states |
| Breadboard and Jumper Wires | Circuit prototyping and connections |


## PPM Thresholds and Logic
The system categorizes air quality into three distinct levels:

* SAFE (0 - 950 PPM): Green LED ON. System normal.
* CAUTION (951 - 1150 PPM): LCD Warning. Potential leak detected.
* DANGER (1151+ PPM): Red LED ON, Buzzer active, Telegram Alert sent to group.

## Telegram Bot Integration
The system is managed through a Telegram Group Chat.

### User Commands (Available to All)
* /status - Get current gas level and system health.
* /history - View recent log of readings.
* /test_alarm - Trigger a short beep to test the buzzer.
* /restart - Reboots the ESP32.
* /help - List all available commands.

### Admin Commands (Restricted)
* /alarm_off - Manually silence the active buzzer.
* /alarm_on - Re-enable the alarm system.
* /reset_alarm - Clear the current alarm state.
* /shutdown - Puts the system into a low-power standby.
* /reset_system - Factory reset software variables.


## Setup and Installation

### 1. Bot Configuration
1. Create a bot via @BotFather on Telegram to get your API Token.
2. Add the bot to your target group and promote it to Admin.
3. Use @userinfobot to retrieve your Group Chat ID and your Admin User ID.

### 2. Software Preparation
Install the following libraries in the Arduino IDE:
* UniversalTelegramBot by Brian Lough
* LiquidCrystal_I2C by Frank de Brabander
* WiFi and WiFiClientSecure (ESP32 Built-in)
* Preferences (ESP32 Built-in)

## Folder Structure
- code/           Arduino source code
- schematics/     Circuit schematic diagram
- hardware/       Photos of the physical build and live demo
- documentation/  Full project report
