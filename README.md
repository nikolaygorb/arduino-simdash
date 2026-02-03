# Digital VFD Sim-Dashboard

Digital instrument cluster for racing and truck simulators with VFD-like visual styling.

## Description

This project is a hardware instrument panel for racing and truck simulators (Assetto Corsa, ETS2, ATS, etc.) with SimHub integration. Uses ESP32-S3 and a color TFT display to show telemetry in the style of vintage vacuum fluorescent displays.

The panel displays main vehicle parameters in real time:
- Tachometer with color zones (green / yellow / red)
- Speedometer
- Gear indicator
- Coolant temperature
- Oil pressure
- Fuel level
- Throttle and brake pedal positions
- Status indicators (turn signals, high beam, cruise control, ABS, etc.)

## Requirements

### Hardware

**Microcontroller:**
- ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)

**Display:**
- TFT SPI 3.5" (480x320), ST7796 controller

**Wiring:**
```
MOSI: GPIO 11  →  Display SDI
SCLK: GPIO 12  →  Display SCK
DC:   GPIO  9  →  Display DC
RST:  GPIO  8  →  Display RESET
CS:   GPIO 10  →  Display CS
LED:  GPIO 13  →  Display LED (PWM)
VCC:  5V
GND:  GND
```

### Software

**Arduino IDE:**
- Arduino IDE 1.8.x or 2.x
- ESP32 board support installed (esp32 by Espressif Systems)

**Libraries:**
- LovyanGFX (version 1.x) - for display control

**Integration:**
- SimHub (free version) - for telemetry transmission
- USB connection (CDC) between PC and ESP32

## Components and Modules

### Main Code Modules

**Display Configuration (LGFX)**
- ST7796 configuration class via SPI
- Bus parameters: 80 MHz write, SPI mode 0
- Hardware SPI support (SPI2_HOST)

**Telemetry Processing**
- Data parsing from SimHub via Serial (USB CDC)
- Format: key-value pairs separated by semicolons
- Universal telemetry support for different simulators

**Visualization**
- VFD glow effect (multi-layer text rendering)
- Color palette in vacuum fluorescent display style
- Smooth animation via EMA filters
- Adaptive tachometer scale with zones (green/yellow/orange/red)

**Status Indicators**
- 9 icons at top of screen: turn signals, ABS, high/low beam, hazards, cruise, low fuel, oil pressure
- Blinking turn signals and hazard lights
- Automatic detection of available game data

**Safety System**
- Watchdog Timer (5 seconds)
- SimHub data timeout (3 seconds)
- System states: BOOT / RUNNING / ERROR

### Screen Layout

```
┌─────────────────────────────────────────────────────┐
│ [←][ABS][LOW][HAZ][HI][LIM][FUEL][OIL][→]          │  Indicators
├─────────────────────────────────────────────────────┤
│ [████████████████████░░░░░░░░░] RPM BAR            │  RPM bar
├─────────────────────────────────────────────────────┤
│ ┌──────────┬────────────────┬──────────┐           │
│ │ COOLANT  │                │  8245    │           │
│ │ [████░░] │       4        │   RPM    │           │
│ │ OIL-P    │                │          │           │
│ │ [██████] │     GEAR       │   235    │           │  Main information
│ │ FUEL     │                │   KMH    │           │
│ │ [██░░░░] │                │          │           │
│ └──────────┴────────────────┴──────────┘           │
├─────────────────────────────────────────────────────┤
│ BRK [████████░░░░░░░░░] THR [░░░░░████████]        │  Pedals
└─────────────────────────────────────────────────────┘
```

## SimHub Setup

In SimHub you need to add a Custom Serial Device with the following JavaScript code (included in the source file comments):

**Location:** Settings → Custom Serial Devices → New Device
- Type: Custom protocol
- COM port: select ESP32
- Speed: 115200 baud (default)

The script automatically formats telemetry packet with all parameters and sends via Serial.

## Usage

1. Upload firmware to ESP32-S3 via Arduino IDE
2. Connect display according to wiring diagram
3. Configure Custom Serial Device in SimHub
4. Launch simulator and SimHub
5. Panel will automatically start displaying telemetry

If no data from SimHub (more than 3 seconds), screen will display connection loss message.

## Features

- Universal support: works with racing simulators (AC, ACC, iRacing) and truck sims (ETS2, ATS)
- Adaptive logic: automatically detects available parameters from game
- Smooth animation: EMA filters to eliminate jitter
- VFD styling: glow effect on dark background
- Resource efficient: sprite buffer rendering (30 FPS)
- Reliable: built-in watchdog and connection loss handling

## Technical Details

**Performance:**
- Refresh rate: ~30 FPS (33ms per frame)
- Baud rate: 115200
- Smoothing: EMA alpha = 0.4

**Color Palette:**
- Main VFD: #04C4CA (cyan)
- Warnings: #FF8C00 (orange)
- Critical: #FF0040 (red)
- High beam: blue
- Throttle: green

---

Project created for simracing enthusiasts. Open for modifications and improvements.
