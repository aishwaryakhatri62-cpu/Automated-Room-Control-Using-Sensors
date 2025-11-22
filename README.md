<p>
  <div>
    # Automated Room Control Using Sensors

**Project:** Automated Room Control Using Sensors
**Author:** Aishwarya Khatri
**Purpose:** A demo-ready Arduino-based system that monitors temperature, humidity, distance (presence), light level and obstacle detection to automatically control a room fan (via relay), lighting, and an indicator LED. Designed for exhibitions and coursework.

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Features](#features)
3. [Hardware Components](#hardware-components)
4. [Pin Mapping & Wiring (Arduino UNO)](#pin-mapping--wiring-arduino-uno)
5. [Power & Safety Notes](#power--safety-notes)
6. [Software / Libraries](#software--libraries)
7. [How to Upload & Run](#how-to-upload--run)
8. [Tinkercad Simulation Notes](#tinkercad-simulation-notes)
9. [Troubleshooting & Common Fixes](#troubleshooting--common-fixes)
10. [Demo Script & Test Plan](#demo-script--test-plan)
11. [Files in this repo](#files-in-this-repo)
12. [License & Credits](#license--credits)

---

## Project Overview

This project uses multiple sensors to automate room control:

* DHT11 for temperature & humidity
* HC-SR04 ultrasonic for presence/distance
* LDR (photoresistor) or LDR module for light detection
* IR obstacle sensor for close-range detection
* 2-channel relay module to switch a DC fan and a light/LED
* 16×2 LCD (I2C) to display system status
* Indicator LED for alerts

The control logic turns the fan on when temperature is above a threshold (or presence + near-threshold), turns lights on when presence is detected in low light, and displays telemetry on the LCD. The design emphasizes non-blocking code and stable sensor reads.

---

## Features

* Real-time display of Temp (°C) and Humidity (%) on I2C LCD
* Presence detection using ultrasonic sensor and IR module
* Light-based decisions using LDR
* Relay-driven DC fan control (external motor power)
* Non-blocking timing (millis) and sensor smoothing (median filter)
* Clear serial debug output for testing

---

## Hardware Components

* Arduino UNO R3
* DHT11 or DHT22 (DHT11 used in this repo)
* HC-SR04 ultrasonic sensor
* Photoresistor (LDR) + 10kΩ resistor (voltage divider) or LDR module
* IR obstacle sensor (digital output) or pushbutton for simulation
* 2-channel relay module (5V)
* DC motor with fan (12V recommended)
* 16×2 LCD with I2C backpack (or plain 16×2 LCD)
* Breadboard, jumper wires, 220Ω resistor (LED), pull-up/pull-down resistors, external motor power supply, diode for motor (if using MOSFET)
* Optional: MOSFET + diode if simulating motor switching without relay

---

## Pin Mapping & Wiring (Arduino UNO)

> Default mapping used by the main code in this repo. Change pins in code if you wire differently.

**Digital**

* DHT data → `D2`
* Ultrasonic TRIG → `D3`
* Ultrasonic ECHO → `D4`
* IR obstacle DO → `D5`
* LDR module digital (if using DO) → `D6` *(if using analog divider use A0 instead and adjust code)*
* LED (indicator) → `D7`
* Relay IN1 (fan) → `D8`
* Relay IN2 (light/LED) → `D9`

**I2C (LCD)**

* SDA → `A4`
* SCL → `A5`
* VCC → `5V`
* GND → `GND`

**Motor power (external)**

* External Supply (+) → Relay COM1
* Relay NO1 → Motor +
* Motor − → External Supply GND
* Connect External Supply GND → Arduino GND (common ground)

**LDR analog divider (if used)**

* Photoresistor between `5V` and Vout
* 10k resistor between Vout and `GND`
* Vout → `A0` (read with `analogRead(A0)`)

---

## Power & Safety Notes

* **Always use a separate power supply for the DC motor/fan** (e.g., 9–12V adapter) — do not power large motors from Arduino 5V.
* **Common ground:** Connect the motor supply GND to Arduino GND when signals rely on shared ground.
* **Relay module JD-VCC:** Some relay modules have a separate coil supply pin (JD-VCC). Make sure coil power is enabled or jumper is set correctly.
* If switching mains AC, get supervision and use appropriately rated relays and isolation — avoid mains in early demos if you are unsure.

---

## Software / Libraries

Install via Arduino IDE Library Manager:

* `DHT sensor library` — Adafruit (or equivalent)
* `Adafruit Unified Sensor` (dependency)
* `LiquidCrystal_I2C` (or use `LiquidCrystal` if using 4-bit wiring)
* Optionally: `NewPing` for ultrasonic (not required; code uses `pulseIn()`)

**Main sketch:** `automated_room_control.ino` (this repo’s main file) — configured for DHT11 by default.

---

## How to Upload & Run

1. Open Arduino IDE.
2. Install libraries listed above.
3. Open `automated_room_control.ino`.
4. Change `LCD_ADDR` if your I2C address is different (`0x27` or `0x3F`). If using DHT22, change `#define DHTTYPE DHT11` accordingly.
5. Select board `Arduino/Genuino Uno` and correct COM port.
6. Upload.
7. Open Serial Monitor at `115200` to view debug logs.

---

## Tinkercad Simulation Notes

* Tinkercad supports most parts but not always the exact I2C backpack; if the I2C LCD is unavailable, use the 16×2 LCD in 4-bit mode and swap to `LiquidCrystal` library (example code included in repo).
* Relay modules may be substituted with MOSFET/transistor arrangements for low-voltage motor switching in the simulator.
* Simulate IR sensor with a pushbutton; simulate LDR with Tinkercad photoresistor slider.

---

## Troubleshooting & Common Fixes

**LCD backlight on but no text**

* Run an I2C scanner to find address (use the scanner sketch in `tools/i2c_scanner.ino`). Update `LCD_ADDR`.

**DHT reads NAN**

* Confirm `DHTTYPE` matches your hardware (DHT11 vs DHT22).
* Add 4.7k–10k pull-up resistor between DATA and VCC if using bare sensor.
* Ensure you read DHT only every ~2s (code respects this).

**Relay doesn't click**

* Ensure relay module VCC & GND are connected. If module has JD-VCC, ensure coil jumper or external JD-VCC connected.
* Confirm `digitalWrite` logic: many modules are **active LOW** (LOW = ON). The code uses active-LOW abstraction — flip logic if your module is active-HIGH.

**Motor doesn’t spin**

* Check wiring: COM -> motor supply +, NO -> motor + terminal, motor − -> supply GND. Ensure motor supply turned on and GND common with Arduino (if using control signals).

---

## Demo Script & Test Plan

1. Power everything (Arduino via USB, motor via external supply).
2. Open Serial Monitor and confirm startup messages.
3. Watch LCD: it should show temperature & humidity; verify DHT reading changes when you warm sensor with your hand.
4. Move an object in front of the HC-SR04 to check distance readings and presence detection.
5. Simulate darkness (cover LDR) and presence — LED and relay-controlled light should respond.
6. Raise temperature (or change threshold) to trigger fan relay; listen for relay click and fan spinning.

---

## Files in this repo

* `automated_room_control.ino` — main Arduino sketch (DHT11 default)
* `readme.md` — this document
* `tools/i2c_scanner.ino` — I2C address detection sketch
* `schematics/` — (optional) wiring diagrams and images
* `assets/ide_screenshot.png` — saved IDE screenshot (uploaded). View it here: `/mnt/data/8e2cfc1e-71f7-450a-9700-6120f83db57d.png`

---

## License & Credits

* MIT License — feel free to reuse and adapt for demonstrations and coursework (please credit the author).
* Libraries used: Adafruit DHT, LiquidCrystal_I2C (see library authors for license details).

---

## Contact / Next Steps

If you want I can:

* Generate a labeled wiring diagram image (Tinkercad-ready) for your exact pin mapping.
* Produce a version of the sketch for Tinkercad (4-bit LCD or MOSFET motor switching).
* Add SD logging or a web interface (ESP8266/ESP32 variant) for remote monitoring.

---

**Good luck at the expo — tell me which diagram or variant you want next and I’ll create it.**

  </div>
</p>
