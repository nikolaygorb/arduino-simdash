# Arduino Sim-Dashboard

Two hardware dashboards for sim racing and truck simulators, both integrated with SimHub over Serial.

## What's Inside

**dashboard_digital.ino** -- Digital dashboard with VFD-style rendering on a TFT screen.
Uses ESP32-S3 and a 3.5" color display. Shows RPM bar, speed, gear, gauges (coolant, oil, fuel), throttle/brake bars, and status icons -- all drawn with a cyan glow effect on black background.

**dashboard_galant.ino** -- Physical analog dashboard in Mitsubishi Galant style.
Uses Arduino Mega 2560 with four stepper motors (speedometer, tachometer, fuel, coolant temp), 28 addressable RGB LEDs for indicators and backlight, a piezo speaker for turn signal clicks, and physical buttons for color cycling and reset.

Both share the same SimHub telemetry protocol and JavaScript code. Both work with racing sims (Assetto Corsa, ACC, iRacing) and truck sims (ETS2, ATS).

## Hardware

### Digital (ESP32)

| Part | Spec |
|------|------|
| MCU | ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM) |
| Display | TFT SPI 3.5" 480x320, ST7796 |
| Connection | USB CDC |

Wiring:
```
MOSI: GPIO 11    SCLK: GPIO 12    DC: GPIO 9
RST:  GPIO  8    CS:   GPIO 10    LED: GPIO 13 (PWM)
VCC: 5V, GND: GND
```

### Galant (Arduino Mega)

| Part | Spec |
|------|------|
| MCU | Arduino Mega 2560 (CH340, USB-C) |
| Motors | 4x X27-168 stepper (via 4x TB6612FNG) |
| LEDs | 28x WS2812B (indicators, shift lights, RGB backlight) |
| Speaker | Passive piezo (turn signal click) |
| Controls | RESET button, COLOR button, B10K potentiometer (brightness) |

Gauge sweeps: Speedometer 260 deg (0-220 km/h), Tachometer 265 deg (0-8000 RPM), Fuel 150 deg, Coolant 130 deg.

## Libraries

- **Digital:** LovyanGFX 1.x
- **Galant:** FastLED

Both need the respective board support packages installed in Arduino IDE (ESP32 / AVR).

## SimHub Setup

1. Open SimHub -> Settings -> Custom Serial Devices -> New Device
2. Set type to Custom protocol, pick the right COM port
3. Paste the JavaScript from the top of either .ino file (they use the same script)
4. Baud rate: 115200

The script sends telemetry as key-value pairs: `KMH:120;RPM:5400;GEAR:3;FUEL:65;...`

Covers speed, RPM, max RPM, gear, fuel, water temp, oil pressure/temp, throttle, brake, plus indicators -- ABS, TC, turn signals, hazards, lights, handbrake, cruise control, shift lights, and truck-specific warnings.

## How It Works

Both dashboards follow the same pattern:
- Parse incoming Serial data from SimHub
- Smooth values with EMA filters to avoid jitter
- Detect what data the game actually provides and fill in sensible defaults where needed
- Handle SimHub disconnection gracefully (timeout 1-3 seconds)
- Watchdog timer for auto-recovery on hang

### Digital-specific
- Renders to a full-screen sprite buffer at ~30 FPS
- VFD glow effect: three-layer text rendering (outer glow, inner glow, core)
- RPM bar with color zones (green / yellow / orange / red)
- Vertical gauges for coolant, oil pressure, fuel on the left
- Horizontal bars for brake and throttle at the bottom

### Galant-specific
- Drives four stepper motors with half-step sequences (1080 steps/rev)
- 4-stage shift light (green -> yellow -> orange -> red strobe at 13 Hz)
- Turn signal speaker clicks (two-tone pulse simulating relay)
- Welcome animation on startup (all needles sweep, LEDs fade)
- 20 RGB backlight color presets cycled with button
- Brightness controlled by potentiometer

## Screen Layout (Digital)

```
+---------------------------------------------------------+
| [<-][ABS][LOW][HAZ][HI][LIM][FUEL][OIL][->]   Icons    |
|---------------------------------------------------------|
| [================----------]                   RPM bar  |
|---------------------------------------------------------|
| COOLANT  |              |  8245 RPM                     |
| OIL-P    |      4       |                               |
| FUEL     |    GEAR      |   235 KMH                     |
|---------------------------------------------------------|
| BRK [========-------] THR [------========]     Pedals   |
+---------------------------------------------------------+
```

## LED Layout (Galant)

28 LEDs total, organized in physical groups:
- Turn signals (2) + high beam (1)
- Speedometer indicators: overheat, check engine, low oil, hazard, low battery
- Tachometer indicators: ABS, TC, low beam/parking, doors, handbrake
- Center: low fuel
- Shift lights (2, dynamic color)
- RGB backlight (3 groups, 12 LEDs total)

---

Built for sim racing enthusiasts. Modify freely.
