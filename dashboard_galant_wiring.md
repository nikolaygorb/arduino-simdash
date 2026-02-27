# Dashboard Galant -- Assembly & Wiring Guide

A step-by-step guide to building the physical Mitsubishi Galant sim-dashboard.

---

## Parts List

| # | Part | Qty | Notes |
|---|------|-----|-------|
| 1 | Arduino Mega 2560 (CH340, USB-C) | 1 | The brain. Any Mega clone works |
| 2 | X27-168 stepper motor | 4 | Micro stepper for instrument gauges |
| 3 | TB6612FNG dual H-bridge driver | 4 | One per motor. Do NOT use L298N -- too bulky |
| 4 | WS2812B LED strip (individual LEDs or strip) | 28 pcs | Addressable RGB, 5V. Cut from a strip or use individual modules |
| 5 | Passive piezo buzzer/speaker | 1 | For turn signal click sound |
| 6 | Tactile push button | 2 | RESET and COLOR buttons |
| 7 | B10K potentiometer | 1 | Linear, for backlight brightness |
| 8 | Jumper wires | lots | Male-male and male-female |
| 9 | USB cable (USB-A to USB-C or USB-B) | 1 | To connect Mega to PC |
| 10 | 5V power supply (optional) | 1 | If LEDs draw too much from USB alone |

---

## Module Overview

There are 5 independent subsystems, all connected to the same Arduino Mega:

```
                    +---------------------------+
                    |     Arduino Mega 2560     |
                    |                           |
  USB from PC ----->| Serial (115200 baud)      |
                    |                           |
                    | D2-D7 -----> TB6612 #1 -----> Stepper Motor #1 (Speedometer)
                    | D8 ---------> STBY (shared by all 4 TB6612 boards)
                    | D9-D13,D46 -> TB6612 #2 -----> Stepper Motor #2 (Tachometer)
                    | D22-D25,D44-D45 -> TB6612 #3 -> Stepper Motor #3 (Fuel)
                    | D26-D29,D50-D51 -> TB6612 #4 -> Stepper Motor #4 (Coolant)
                    |                           |
                    | D30 -------> WS2812B LED chain (28 LEDs)
                    | D47 -------> Piezo speaker
                    | D48 -------> RESET button
                    | D49 -------> COLOR button
                    | A0 --------> Potentiometer (brightness)
                    +---------------------------+
```

---

## 1. Stepper Motors (X27-168) via TB6612FNG

Each X27-168 motor has 4 pins (two coils: A and B). Each TB6612FNG board controls both coils of one motor.

### How a TB6612FNG board is wired

Every TB6612 board has these pins:

| TB6612 Pin | What it does |
|------------|-------------|
| AIN1, AIN2 | Direction control for coil A |
| BIN1, BIN2 | Direction control for coil B |
| PWMA | Speed/power for coil A (connect to PWM pin or just HIGH) |
| PWMB | Speed/power for coil B (connect to PWM pin or just HIGH) |
| STBY | Standby -- must be HIGH for the driver to work |
| AOUT1, AOUT2 | Output to motor coil A |
| BOUT1, BOUT2 | Output to motor coil B |
| VM | Motor power supply -- connect to 5V |
| VCC | Logic power -- connect to 5V |
| GND | Ground -- connect to Arduino GND |

### Wiring Table

**TB6612 #1 -- Speedometer**

| TB6612 Pin | Arduino Pin |
|------------|-------------|
| AIN1 | D2 |
| AIN2 | D3 |
| BIN1 | D4 |
| BIN2 | D7 |
| PWMA | D5 |
| PWMB | D6 |
| STBY | D8 (shared) |
| AOUT1, AOUT2 | Motor #1 coil A |
| BOUT1, BOUT2 | Motor #1 coil B |
| VM, VCC | 5V |
| GND | GND |

**TB6612 #2 -- Tachometer**

| TB6612 Pin | Arduino Pin |
|------------|-------------|
| AIN1 | D9 |
| AIN2 | D10 |
| BIN1 | D11 |
| BIN2 | D12 |
| PWMA | D13 |
| PWMB | D46 |
| STBY | D8 (shared) |
| AOUT1, AOUT2 | Motor #2 coil A |
| BOUT1, BOUT2 | Motor #2 coil B |
| VM, VCC | 5V |
| GND | GND |

**TB6612 #3 -- Fuel Level**

| TB6612 Pin | Arduino Pin |
|------------|-------------|
| AIN1 | D22 |
| AIN2 | D23 |
| BIN1 | D24 |
| BIN2 | D25 |
| PWMA | D44 |
| PWMB | D45 |
| STBY | D8 (shared) |
| AOUT1, AOUT2 | Motor #3 coil A |
| BOUT1, BOUT2 | Motor #3 coil B |
| VM, VCC | 5V |
| GND | GND |

**TB6612 #4 -- Coolant Temperature**

| TB6612 Pin | Arduino Pin |
|------------|-------------|
| AIN1 | D26 |
| AIN2 | D27 |
| BIN1 | D28 |
| BIN2 | D29 |
| PWMA | D50 |
| PWMB | D51 |
| STBY | D8 (shared) |
| AOUT1, AOUT2 | Motor #4 coil A |
| BOUT1, BOUT2 | Motor #4 coil B |
| VM, VCC | 5V |
| GND | GND |

**Important:** D50 and D51 are NOT PWM-capable pins on the Mega. The code uses `digitalWrite(HIGH)` instead of `analogWrite()` for these. This is fine -- the motor still works at full power.

### STBY pin (shared)

All four TB6612 STBY pins connect to the same Arduino pin **D8**. When D8 is HIGH, all four drivers are enabled. You can wire them together on a breadboard or solder a single wire that branches to all four boards.

```
D8 ---+--- TB6612 #1 STBY
      +--- TB6612 #2 STBY
      +--- TB6612 #3 STBY
      +--- TB6612 #4 STBY
```

### Motor wiring tip

X27-168 motors have 4 very thin pins. Coil A is pins 1-2, coil B is pins 3-4 (or the reverse -- if the needle moves backward, swap one pair). If unsure, try it -- wrong coil pairing just makes the motor vibrate instead of rotating.

---

## 2. WS2812B LED Chain

All 28 LEDs are wired in one continuous chain (data out of LED N goes to data in of LED N+1).

| Connection | Where |
|------------|-------|
| Data In (first LED) | Arduino **D30** |
| VCC (all LEDs) | 5V |
| GND (all LEDs) | GND |

**Power note:** 28 WS2812B LEDs at full white draw about 1.7A. USB alone can't supply that. In practice, the dashboard rarely lights all LEDs at full brightness, so USB power often works. If you see flickering or brownouts, add an external 5V supply connected to the LED strip's VCC/GND (and connect its GND to Arduino GND too).

**Optional but recommended:** Place a 300-470 ohm resistor between D30 and the first LED's data input. Place a 100-1000 uF capacitor across the LED strip's VCC and GND. This protects against voltage spikes.

### LED order on the chain

The LEDs are arranged in this exact order along the strip. The physical layout alternates between indicators and backlight groups:

```
Index   Function                Color when active
-----   --------                -----------------
  0     Left Turn Signal        Green
  1     High Beam               Blue
  2     Right Turn Signal       Green
  3-6   Backlight Group 1       RGB (user-selectable, 20 presets)
  7     Coolant Overheat        Orange
  8     Check Engine            Red
  9     Low Oil Pressure        Orange
 10     Hazard Lights           Red
 11     Low Battery             Red
 12-13  Backlight Group 2       RGB (same color as Group 1)
 14     Low Fuel                Orange
 15     Shift Light 1           Green / Yellow / Orange / Red (dynamic)
 16     Shift Light 2           Green / Yellow / Orange / Red (dynamic)
 17     ABS                     Yellow
 18     TC (Traction Control)   Yellow
 19     Low Beam / Parking      Green
 20     Doors Open              Yellow
 21     Handbrake               Yellow
 22-27  Backlight Group 3       RGB (same color as Group 1)
```

When mounting on a physical dashboard, the groups map like this:
- LEDs 0-2: top center (turn signals + high beam)
- LEDs 3-6: backlight behind left gauge cluster
- LEDs 7-11: under speedometer
- LEDs 12-13: backlight behind center
- LED 14: center (fuel warning)
- LEDs 15-16: shift lights (visible near tachometer)
- LEDs 17-21: under tachometer
- LEDs 22-27: backlight behind right gauge cluster

---

## 3. Piezo Speaker (Turn Signal Clicks)

| Connection | Where |
|------------|-------|
| Positive (+) | Arduino **D47** |
| Negative (-) | GND |

That's it. The code generates short pulses (microsecond-level) to simulate a relay click sound when turn signals change state.

---

## 4. Buttons

Both buttons use Arduino's internal pull-up resistors, so no external resistors are needed. Just connect one side to the Arduino pin and the other side to GND.

**RESET button:**
```
D48 ----[button]---- GND
```
Pressing it re-runs the welcome animation and resets everything.

**COLOR button:**
```
D49 ----[button]---- GND
```
Each press cycles to the next backlight color preset (20 total, starting from warm white).

---

## 5. Brightness Potentiometer

A B10K (10 kilohm linear) potentiometer wired as a voltage divider:

```
5V ---- [pot pin 1]
         [pot pin 2 (wiper)] ---- Arduino A0
GND --- [pot pin 3]
```

Turning it adjusts the RGB backlight brightness from 10% to 80%.

---

## Power Wiring Summary

```
Arduino 5V ----+---- All TB6612 VM pins
               +---- All TB6612 VCC pins
               +---- WS2812B VCC
               +---- Potentiometer pin 1

Arduino GND ---+---- All TB6612 GND pins
               +---- WS2812B GND
               +---- Piezo speaker (-)
               +---- Button (RESET) one leg
               +---- Button (COLOR) one leg
               +---- Potentiometer pin 3
```

If using external 5V power for the LEDs, connect its GND to Arduino GND (common ground).

---

## Assembly Checklist

1. [ ] Mount 4 stepper motors into your dashboard housing
2. [ ] Wire each motor to its TB6612 board (AOUT/BOUT to motor coils)
3. [ ] Wire all 4 TB6612 boards to Arduino (see tables above)
4. [ ] Connect all STBY pins together to D8
5. [ ] Connect all TB6612 power pins (VM, VCC to 5V; GND to GND)
6. [ ] Prepare WS2812B LED chain (28 LEDs in correct order)
7. [ ] Connect LED data to D30 (with optional 330 ohm resistor)
8. [ ] Connect LED power (5V + GND)
9. [ ] Wire piezo speaker between D47 and GND
10. [ ] Wire RESET button between D48 and GND
11. [ ] Wire COLOR button between D49 and GND
12. [ ] Wire potentiometer: 5V -- wiper to A0 -- GND
13. [ ] Connect Arduino to PC via USB
14. [ ] Upload dashboard_galant.ino via Arduino IDE
15. [ ] Power on -- you should see the welcome animation (all needles sweep, LEDs light up)
16. [ ] Set up SimHub Custom Serial Device (see README)

---

## Troubleshooting

**Motor vibrates but doesn't rotate:** Swap one coil pair (e.g., swap AOUT1 and AOUT2 connections on that motor).

**Motor turns the wrong direction:** Swap both coil pairs, or reverse the step direction in code.

**LEDs show wrong colors or flicker:** Check your LED type is WS2812B (not WS2811 or SK6812). Make sure GND is shared between LED strip and Arduino.

**No serial data from SimHub:** Check COM port in SimHub settings. Make sure baud rate is 115200. Check that the JavaScript is pasted in the Custom Serial Device script field.

**Arduino resets randomly:** The watchdog timer reboots if the main loop hangs for 4+ seconds. This usually means a wiring short is causing the MCU to stall. Check for shorts, especially on motor driver pins.

**LEDs are dim or brownout:** Not enough current from USB. Add an external 5V 2A power supply to the LED strip.
