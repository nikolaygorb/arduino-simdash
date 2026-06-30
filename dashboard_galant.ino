/*
  Mitsubishi Galant Dashboard - SimHub Integration

  Hardware:
  - Arduino Mega 2560 CH340 (USB-C)
  - 4x X27-168 stepper motors (Speedometer, Tachometer, Fuel Level, Coolant Temp)
  - 4x TB6612FNG dual H-bridge drivers (1 per motor)
  - 28x WS2812B RGB LEDs (5V, addressable)
  - 1x Passive Piezo Speaker (D47, turn signal click sound)
  - 1x RESET button (D48)
  - 1x COLOR button (D49) - cycle RGB backlight colors
  - 1x B10K Potentiometer (A0) - backlight brightness control

  Gauge Angles (sweep from zero stop, see *_MAX_ANGLE constants):
  - Speedometer: 260 deg sweep (0-220 km/h)
  - Tachometer:  265 deg sweep (0-8000 rpm)
  - Fuel Level:  150 deg sweep (E -> F)
  - Coolant Temp: 130 deg sweep (C -> H)

  Dashboard Indicators (via WS2812B):
  Under Tachometer (5 indicators):
  - Handbrake / Parking Brake (red)
  - Doors Open (orange, when speed 0-2 km/h)
  - Low Beam Headlights (green)
  - Parking Lights (running lights, green)
  - ABS / TC (Traction Control, yellow)

  Under Speedometer (5 indicators):
  - Low Battery (orange)
  - Hazard Lights indicator (red)
  - Low Oil Pressure (red)
  - Check Engine (red)
  - Coolant Overheat (red)

  Center (1 indicator):
  - Fuel Low (orange)

  Shift Light (2 LEDs, dynamic 4-stage):
  - <70% RPM: OFF
  - 70-88% RPM: Green (solid)
  - 88-92% RPM: Yellow (solid)
  - 92-96% RPM: Orange (solid)
  - ≥96% RPM: Red (fast blink 75ms / 13 Hz strobe)

  Turn Signals (2 LEDs, blink 500ms with speaker click):
  - Left Turn Signal (green, 1 LED)
  - Right Turn Signal (green, 1 LED)

  Other:
  - High Beam Headlights (blue)
  - RGB Backlight (12 LEDs in 3 groups, warm white default, adjustable via COLOR button)

  SimHub Protocol:
  - One-way passive reception on Serial (115200 baud)
  - Format: KEY1:VALUE1;KEY2:VALUE2;...\n
  - Required: KMH, RPM, optional indicators
  - Computed values when data unavailable (IGNITION, CHECK_ENGINE, ENGINE_RUNNING, etc.)

  Features:
  - Exponential smoothing (alpha=0.3) with jump protection
  - SimHub timeout (1s) - needles freeze on disconnect
  - Watchdog timer (4s) - auto-reboot on hang
  - Fast shift light strobe (75ms, 13 Hz) at critical RPM
  - Welcome animation: all 4 needles sweep min→max→min in parallel, RGB LEDs fade in/out
  - RGB backlight color cycling via COLOR button
  - Two-tone speaker click (800Hz/600Hz) synchronized with turn signals


  SimHub JavaScript:
 Universal Telemetry Parser
function isOn(v) {
  return v === true || v === 1 || v === "1";
}
var oilPress = $prop('OilPressure');
// Convert PSI to Bar (if value is suspiciously high)
if (oilPress > 15) { oilPress = oilPress / 14.5038; }

var handbrake =
    isOn($prop('Handbrake')) ||
    isOn($prop('DataCorePlugin.GameData.NewData.TruckValues.ParkingBrakeOn')) ||
    isOn($prop('TruckValues_ParkingBrakeOn')) ||
    isOn($prop('DataCorePlugin.GameData.Handbrake')) ||
    isOn($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.MotorValues.BrakeValues.ParkingBrake'));

var parkingLights =
    isOn($prop('DataCorePlugin.GameData.NewData.Lights.ParkingOn')) ||
    isOn($prop('DataCorePlugin.GameData.NewData.TruckValues.ParkingBrakeOn')) ||
    isOn($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.LightsValues.Parking'));

var lowBeam =
    isOn($prop('DataCorePlugin.GameData.NewData.Lights.LowBeamOn')) ||
    isOn($prop('DataCorePlugin.GameData.NewData.TruckValues.LightsLowBeamOn')) ||
    isOn($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.LightsValues.BeamLow'));

var highBeam =
    isOn($prop('DataCorePlugin.GameData.NewData.Lights.HighBeamOn')) ||
    isOn($prop('DataCorePlugin.GameData.NewData.TruckValues.LightsHighBeamOn')) ||
    isOn($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.LightsValues.BeamHigh'));

var hazards = isOn($prop('DataCorePlugin.GameData.NewData.Lights.HazardLightsOn')) ||
    isOn($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.LightsValues.HazardWarningLights'));

var cruise =
    isOn($prop('PitLimiterOn')) ||
    isOn($prop('CruiseControlActive')) ||
    isOn($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.DashboardValues.CruiseControl'));

var abs = isOn($prop('DataCorePlugin.GameData.ABSActive')) ||
    isOn($prop('DataCorePlugin.GameData.ABSActive'));

var tc = isOn($prop('DataCorePlugin.GameData.TCActive'));

var msg =
  "KMH:" + Math.round($prop('SpeedKmh')) + ";" +
  "RPM:" + Math.round($prop('Rpms')) + ";" +
  "MAXRPM:" + Math.round($prop('CarSettings_MaxRPM')) + ";" +
  "GEAR:" + $prop('Gear') + ";" +
  "FUEL:" + Math.round($prop('DataCorePlugin.Computed.Fuel_Percent')) + ";" +
  "WATER:" + Math.round($prop('WaterTemperature')) + ";" +
  "OILPRESS:" + oilPress.toFixed(1) + ";" +
  "OILTEMP:" + Math.round($prop('OilTemperature')) + ";" +
  "THROTTLE:" + Math.round($prop('Throttle')) + ";" +
  "BRAKE:" + Math.round($prop('Brake')) + ";" +
  "ABS:" + (abs ? "1" : "0") + ";" +
  "TC:" + (tc ? "1" : "0") + ";" +

  "PARKLIGHT:" + (parkingLights ? "1" : "0") + ";" +
  "LOWBEAM:" + (lowBeam ? "1" : "0") + ";" +
  "HIGHBEAM:" + (highBeam  ? "1" : "0") + ";" +
  "HAZARD:" + (hazards ? "1" : "0") + ";" +
  "IGNITION:" + ($prop('DataCorePlugin.GameData.EngineIgnitionOn') ? "1" : "0") + ";" +

  "LEFTTURN:" + ($prop('TurnIndicatorLeft') ? "1" : "0") + ";" +
  "RIGHTTURN:" + ($prop('TurnIndicatorRight') ? "1" : "0") + ";" +
  "HANDBRAKE:" + (handbrake ? "1" : "0") + ";" +
  "CRUISE:" + (cruise ? "1" : "0") + ";" +
  "SHIFTLIGHT1:" + ($prop('DataCorePlugin.GameData.CarSettings_RPMShiftLight1') ? "1" : "0") + ";" +
  "SHIFTLIGHT2:" + ($prop('DataCorePlugin.GameData.CarSettings_RPMShiftLight2') ? "1" : "0") + ";" +
  "WARNING_AIR_PRESS:" +($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.DashboardValues.WarningValues.AirPressure') ? "1" : "0") + ";" +
  "WARNING_OIL_PRESS:" + ($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.DashboardValues.WarningValues.OilPressure') ? "1" : "0") + ";" +
  "WARNING_BATTERY_VOLT:" + ($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.DashboardValues.WarningValues.BatteryVoltage') ? "1" : "0") + ";" +
  "WARNING_WATER_TEMP:" + ($prop('DataCorePlugin.GameRawData.TruckValues.CurrentValues.DashboardValues.WarningValues.WaterTemperature') ? "1" : "0") + ";" +
  "\n";

return msg;

*/

#include <avr/wdt.h> // Watchdog timer
#include <FastLED.h> // WS2812B control

// --------------------------- PINOUT ---------------------------

// TB6612 Motor Driver #1 (Speedometer)
#define TB1_AIN1 2
#define TB1_AIN2 3
#define TB1_BIN1 4
#define TB1_BIN2 7
#define TB1_PWMA 5
#define TB1_PWMB 6

// TB6612 Motor Driver #2 (Tachometer)
#define TB2_AIN1 9
#define TB2_AIN2 10
#define TB2_BIN1 11
#define TB2_BIN2 12
#define TB2_PWMA 13
#define TB2_PWMB 46

// TB6612 Motor Driver #3 (Fuel Level)
#define TB3_AIN1 22
#define TB3_AIN2 23
#define TB3_BIN1 24
#define TB3_BIN2 25
#define TB3_PWMA 44 // Changed from D6 to avoid conflict with TB1
#define TB3_PWMB 45 // Changed from D7 to avoid conflict with TB1

// TB6612 Motor Driver #4 (Coolant Temperature)
#define TB4_AIN1 26
#define TB4_AIN2 27
#define TB4_BIN1 28
#define TB4_BIN2 29
#define TB4_PWMA 50 // Digital pin (PWM not required - always HIGH to avoid TB1 conflict)
#define TB4_PWMB 51 // Digital pin (PWM not required - always HIGH to avoid TB1 conflict)

// Shared standby for all TB drivers
#define TB_STBY 8

// WS2812B LED Strip
#define LED_DATA_PIN 30
#define NUM_LEDS 28 // Total: 0-2 (indicators), 3-6 (backlight1), 7-14 (indicators), 15-16 (backlight2), 17-21 (indicators), 22-27 (backlight3)
CRGB leds[NUM_LEDS];

// Speaker (turn signal click sound - replaces relay)
#define SPEAKER_PIN 47

// Buttons
#define RESET_BTN_PIN 48
#define COLOR_BTN_PIN 49

// Potentiometer (backlight brightness control)
#define BRIGHTNESS_POT_PIN A0 // B10K potentiometer

// --------------------------- MOTOR PARAMETERS ---------------------------
const int STEPS_PER_REV = 1080; // X27-168 standard 600

// Angle definitions (degrees from horizontal right = 0°)
const float SPEED_MIN_ANGLE = 0.0f;   // 210.0f;
const float SPEED_MAX_ANGLE = 260.0f; // 360.0f
const float RPM_MIN_ANGLE = 0.0f;     // 210.0
const float RPM_MAX_ANGLE = 265.0f;   // 350.0
const float FUEL_MIN_ANGLE = 0.0f;    // 230.0f
const float FUEL_MAX_ANGLE = 150.0f;  // 320
const float TEMP_MIN_ANGLE = 0.0f;    // 230
const float TEMP_MAX_ANGLE = 130.0f;  // 310

// Convert angles to steps
const int SPEED_MIN_STEPS = int(round(STEPS_PER_REV * (SPEED_MIN_ANGLE / 360.0f)));
const int SPEED_MAX_STEPS = int(round(STEPS_PER_REV * (SPEED_MAX_ANGLE / 360.0f)));
const int RPM_MIN_STEPS = int(round(STEPS_PER_REV * (RPM_MIN_ANGLE / 360.0f)));
const int RPM_MAX_STEPS = int(round(STEPS_PER_REV * (RPM_MAX_ANGLE / 360.0f)));
const int FUEL_MIN_STEPS = int(round(STEPS_PER_REV * (FUEL_MIN_ANGLE / 360.0f)));
const int FUEL_MAX_STEPS = int(round(STEPS_PER_REV * (FUEL_MAX_ANGLE / 360.0f)));
const int TEMP_MIN_STEPS = int(round(STEPS_PER_REV * (TEMP_MIN_ANGLE / 360.0f)));
const int TEMP_MAX_STEPS = int(round(STEPS_PER_REV * (TEMP_MAX_ANGLE / 360.0f)));

// Step sequence (half-step 8-phase for X27-168)
const uint8_t STEP_SEQ[8][4] = {
    {1, 0, 1, 0},
    {0, 0, 1, 0},
    {0, 1, 1, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1},
    {1, 0, 0, 0}};
const int SEQ_LEN = 8;

// Motor current states
int seqIndexSpeed = 0, seqIndexRpm = 0, seqIndexFuel = 0, seqIndexTemp = 0;
int curSpeedSteps = SPEED_MIN_STEPS;
int curRpmSteps = RPM_MIN_STEPS;
int curFuelSteps = FUEL_MIN_STEPS;
int curTempSteps = TEMP_MIN_STEPS;
unsigned long lastStepTimeSpeed = 0, lastStepTimeRpm = 0, lastStepTimeFuel = 0, lastStepTimeTemp = 0;

const unsigned long STEP_INTERVAL_FAST = 3; // ms (was 2)
const unsigned long STEP_INTERVAL_SLOW = 5; // ms (was 6)

// --------------------------- SIMHUB DATA ---------------------------
// Raw telemetry values
float curKmh = -1.0f; // -1 = no data

float curRpm = -1.0f;

float curFuel = -1.0f; // 0-100%

float curWaterTemp = -1.0f; // °C

const float LAST_GOOD_KMH = 30.0f; // Last good speed for jump protection

float curOilPress = -1.0f; // bar

// Extreme event: force needles to zero (like welcome animation)
bool forceSpeedToZero = false;
bool forceRpmToZero = false;

// Indicators (boolean states from SimHub or computed)
bool indLeftTurn = false;
bool indRightTurn = false;
bool indHazard = false;
bool indLowBeamOrParking = false;
bool indHighBeam = false;
bool indCheckEngine = false;
bool indLowOilPressure = false;
bool indHandbrake = false;
bool indOverheat = false;
bool indABS = false;
bool indTC = false;
bool indCruise = false;
bool indEngineRunning = false;
bool indFuelLow = false;
bool indDoorsOpen = false;
bool warningAirPress = false;
bool warningOilPress = false;
bool warningBatteryVolt = false;
bool warningWaterTemp = false;

// Last good values for jump protection
float lastGoodKmh = -1.0f;
float lastGoodRpm = -1.0f;
float lastGoodWaterTemp = -1.0f;
float lastGoodOilPress = -1.0f;

// Validation ranges
const float SPEED_MIN = 0.0f, SPEED_MAX_POSS = 400.0f; // Accept up to (protects from garbage data)
const float RPM_MIN = 0.0f, RPM_MAX_POSS = 25000.0f;   // Accept up to
const float FUEL_MIN = 0.0f, FUEL_MAX = 100.0f;
const float TEMP_MIN = -40.0f, TEMP_MAX = 180.0f;
const float OILP_MIN = 0.0f, OILP_MAX = 20.0f;

// Jump protection thresholds
// Jump protection (increased for fast response)
const float MAX_JUMP_SPEED = 350.0f; // km/h per packet (was 50)
const float MAX_JUMP_RPM = 12000.0f; // rpm per packet (was 1200)
const float MAX_JUMP_FUEL = 85.0f;   // was 10
const float MAX_JUMP_TEMP = 80.0f;   // was 8
const float MAX_JUMP_OILP = 5.0f;    // was 2

// EMA smoothing for low-speed deceleration (realistic instrument inertia)
const float SMOOTH_ALPHA_LOW = 0.65f;    // Alpha for EMA at low speeds
const float LOW_SPEED_THRESHOLD = 30.0f; // Apply EMA only when decelerating below 30 km/h
const float SPEED_ZERO_SNAP = 2.0f;      // Below this (km/h) snap speed to exact 0 (needle can't show <2)
const float BIG_DROP_SPEED = 25.0f;      // Drop >this (km/h) in one packet = real event, bypass EMA (no lag)
const float LOW_RPM_THRESHOLD = 800.0f;  // Apply EMA only when decelerating below 800 RPM

// Fixed gauge limits (Mitsubishi Galant physical gauge)
const int GAUGE_MAX_SPEED = 220; // km/h
const int GAUGE_MAX_RPM = 8000;  // rpm (physical gauge maximum)

// Shift light RPM zone thresholds (percent of MaxRPM)
const float RPM_ZONE_GREEN_START = 70.0f;  // Green zone starts at 70%
const float RPM_ZONE_YELLOW_START = 88.0f; // Yellow zone starts at 88%
const float RPM_ZONE_ORANGE_START = 92.0f; // Orange zone starts at 92%
const float RPM_ZONE_RED_START = 96.0f;    // Red zone starts at 96% (critical)

// Dynamic shift lights from SimHub (updated per car)
float curMaxRPM = 8000.0f;          // Current car max RPM (from SimHub)
bool indShiftLight = false;         // Shift light state (single LED)
CRGB shiftLightColor = CRGB::Black; // Current shift light color

// FastLED FPS control
unsigned long lastLEDUpdate = 0;
const unsigned long LED_UPDATE_INTERVAL = 90; // 40 FPS 60 OK(1000ms / 40 = 25ms)

// Default "nice" values when data unavailable (but SimHub connected)
const float DEFAULT_FUEL_LEVEL = 75.0f; // 75% - realistic race start
const float DEFAULT_WATER_TEMP = 90.0f; // 90°C - normal operating temp

// Light indicators fallback detection flags
bool parkingSupportedByGame = false;
bool lowBeamSupportedByGame = false;
bool highBeamSupportedByGame = false;

// Telemetry support detection flags
bool fuelSupportedByGame = false;
bool waterTempSupportedByGame = false;

// SimHub timeout
unsigned long lastSimhubPacket = 0;
const unsigned long SIMHUB_TIMEOUT_MS = 1000;

// Watchdog
const unsigned long WDT_RESET_INTERVAL = 100;
unsigned long lastWdtReset = 0;

// Free-RAM diagnostic (logs every 10s to detect heap fragmentation over time)
unsigned long lastRamReport = 0;
const unsigned long RAM_REPORT_INTERVAL = 10000;

// Serial buffer (char array for stability, not String!)
const int SERIAL_MAX_BUFFER = 512; // Increased to handle longer SimHub strings
static char serialBuf[SERIAL_MAX_BUFFER];
static uint16_t serialBufIdx = 0;
unsigned long serial_overflow_count = 0;
bool serialReady = false; // Block data reception until welcome animation completes

// System state
enum SYS_STATE
{
  STATE_WELCOME,
  STATE_WAIT_SIMHUB,
  STATE_RUNNING
};
SYS_STATE sysState = STATE_WELCOME;

// Buttons debounce
unsigned long reset_last = 0;
unsigned long color_last = 0;
const unsigned long BTN_DEBOUNCE_MS = 300;

// RGB backlight color cycling
int colorPresetIndex = 0;
const int NUM_COLOR_PRESETS = 20;
CRGB colorPresets[NUM_COLOR_PRESETS] = {
    CRGB(255, 180, 100), // 1. Warm White (incandescent lamp) - DEFAULT
    CRGB(255, 255, 255), // 2. Cool White
    CRGB(255, 0, 0),     // 3. Red
    CRGB(255, 64, 0),    // 4. Red-Orange
    CRGB(255, 128, 0),   // 5. Orange
    CRGB(255, 200, 0),   // 6. Amber
    CRGB(255, 255, 0),   // 7. Yellow
    CRGB(128, 255, 0),   // 8. Lime Green
    CRGB(0, 255, 0),     // 9. Green
    CRGB(0, 255, 128),   // 10. Spring Green
    CRGB(0, 255, 255),   // 11. Cyan
    CRGB(0, 128, 255),   // 12. Sky Blue
    CRGB(0, 0, 255),     // 13. Blue
    CRGB(64, 0, 255),    // 14. Indigo
    CRGB(128, 0, 255),   // 15. Purple
    CRGB(192, 0, 255),   // 16. Violet
    CRGB(255, 0, 255),   // 17. Magenta
    CRGB(255, 0, 128),   // 18. Pink
    CRGB(255, 100, 180), // 19. Hot Pink
    CRGB(255, 50, 50)    // 20. Coral
};
CRGB currentBacklightColor = colorPresets[0];

// Shift light blink timing (fast strobe for critical RPM)
unsigned long shiftLightLastBlink = 0;
const unsigned long SHIFT_LIGHT_INTERVAL = 75; // ms (13 Hz fast strobe)
bool shiftLightBlinkState = false;

// Turn signal speaker click tracking (no blink state - SimHub sends 0/1 directly)
bool lastTurnSignalState = false; // Track state changes for click sound

// LED assignments (physical layout on dashboard)
// Physical layout: 0-2 (indicators), 3-6 (backlight1), 7-14 (indicators), 15-16 (backlight2), 17-21 (indicators), 22-27 (backlight3)

// Turn signals + High beam (0-2)
const int LED_LEFT_TURN = 0;  // LED 0: Left turn signal (green)
const int LED_HIGH_BEAM = 1;  // LED 1: High beam (blue)
const int LED_RIGHT_TURN = 2; // LED 2: Right turn signal (green)

// Backlight 1 (3-6)
const int LED_BACKLIGHT_1_START = 3; // 4 LEDs: 3-6 (first backlight group)
const int LED_BACKLIGHT_1_COUNT = 4;

// Under Speedometer indicators (7-11)
const int LED_OVERHEAT = 7;     // LED 7: Overheat (orange)
const int LED_CHECK_ENGINE = 8; // LED 8: Check engine (red)
const int LED_LOW_OIL = 9;      // LED 9: Low oil pressure (orange)
const int LED_HAZARD_IND = 10;  // LED 10: Hazard lights (red)
const int LED_LOW_BATTERY = 11; // LED 11: Low battery (red)

// Backlight 2 (12-13)
const int LED_BACKLIGHT_2_START = 12; // 2 LEDs: 12-13 (second backlight group)
const int LED_BACKLIGHT_2_COUNT = 2;

// Center indicator (14)
const int LED_FUEL_LOW = 14; // LED 14: Low fuel level (orange)

// Shift lights (15-16)
const int LED_SHIFT_LIGHT_1 = 15; // LED 15: Shift light 1 (dynamic color)
const int LED_SHIFT_LIGHT_2 = 16; // LED 16: Shift light 2 (dynamic color)

// Under Tachometer indicators (17-21)
const int LED_ABS = 17;                 // LED 17: ABS (yellow)
const int LED_TC = 18;                  // LED 18: TC (yellow)
const int LED_LOW_BEAM_OR_PARKING = 19; // LED 19: Low beam + parking lights (green)
const int LED_DOORS_OPEN = 20;          // LED 20: Doors open (yellow)
const int LED_HANDBRAKE = 21;           // LED 21: Handbrake (yellow)

// Backlight 3 (22-27)
const int LED_BACKLIGHT_3_START = 22; // 6 LEDs: 22-27 (third backlight group)
const int LED_BACKLIGHT_3_COUNT = 6;

// Backlight brightness control
const int BRIGHTNESS_MIN_PERCENT = 10; // 10% minimum (always visible)
const int BRIGHTNESS_MAX_PERCENT = 80; // 80% maximum
int currentBrightnessPct = 100;        // Default 100% (full brightness)
unsigned long lastBrightnessRead = 0;
const unsigned long BRIGHTNESS_READ_INTERVAL = 100; // Read every 100ms

// --------------------------- UTILITY FUNCTIONS ---------------------------
// Free SRAM in bytes: measures the gap between the heap top and the stack.
// Read-only diagnostic, does not touch the motor/serial hot path.
extern unsigned int __heap_start;
extern void *__brkval;
int freeMemory()
{
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

bool inRangeFloat(float v, float minv, float maxv)
{
  return (v >= minv && v <= maxv);
}

bool jumpOK(float newVal, float lastGood, float maxJump)
{
  if (lastGood < 0)
    return true; // first value always OK
  return (abs(newVal - lastGood) <= maxJump);
}

// EMA smoothing (only for low-speed deceleration)
float smooth(float oldVal, float newVal, float alpha)
{
  return alpha * oldVal + (1.0f - alpha) * newVal;
}

// Speaker click sound (replaces relay)
void playClick(bool isOn)
{
  // Two-tone click to simulate relay sound
  int pulses = isOn ? 3 : 1; // Make ON pulses slightly stronger, OFF pulses shorter

  for (int i = 0; i < pulses; i++)
  {
    digitalWrite(SPEAKER_PIN, HIGH);
    delayMicroseconds(random(100, 300)); // Very short pulse
    digitalWrite(SPEAKER_PIN, LOW);
    delayMicroseconds(random(200, 600)); // Pause between micro-bounce pulses
  }
}

// --------------------------- MOTOR LOW-LEVEL ---------------------------
void applyStepStateToMotor(int motorId, int seqIndex)
{
  uint8_t a1 = STEP_SEQ[seqIndex][0];
  uint8_t a2 = STEP_SEQ[seqIndex][1];
  uint8_t b1 = STEP_SEQ[seqIndex][2];
  uint8_t b2 = STEP_SEQ[seqIndex][3];

  switch (motorId)
  {
  case 0: // Speedometer
    digitalWrite(TB1_AIN1, a1 ? HIGH : LOW);
    digitalWrite(TB1_AIN2, a2 ? HIGH : LOW);
    digitalWrite(TB1_BIN1, b1 ? HIGH : LOW);
    digitalWrite(TB1_BIN2, b2 ? HIGH : LOW);
    analogWrite(TB1_PWMA, 255);
    analogWrite(TB1_PWMB, 255);
    break;
  case 1: // Tachometer
    digitalWrite(TB2_AIN1, a1 ? HIGH : LOW);
    digitalWrite(TB2_AIN2, a2 ? HIGH : LOW);
    digitalWrite(TB2_BIN1, b1 ? HIGH : LOW);
    digitalWrite(TB2_BIN2, b2 ? HIGH : LOW);
    analogWrite(TB2_PWMA, 255);
    analogWrite(TB2_PWMB, 255);
    break;
  case 2: // Fuel Level
    digitalWrite(TB3_AIN1, a1 ? HIGH : LOW);
    digitalWrite(TB3_AIN2, a2 ? HIGH : LOW);
    digitalWrite(TB3_BIN1, b1 ? HIGH : LOW);
    digitalWrite(TB3_BIN2, b2 ? HIGH : LOW);
    analogWrite(TB3_PWMA, 255);
    analogWrite(TB3_PWMB, 255);
    break;
  case 3: // Coolant Temperature
    digitalWrite(TB4_AIN1, a1 ? HIGH : LOW);
    digitalWrite(TB4_AIN2, a2 ? HIGH : LOW);
    digitalWrite(TB4_BIN1, b1 ? HIGH : LOW);
    digitalWrite(TB4_BIN2, b2 ? HIGH : LOW);
    digitalWrite(TB4_PWMA, HIGH); // D50 is NOT PWM - use digitalWrite
    digitalWrite(TB4_PWMB, HIGH); // D51 is NOT PWM - use digitalWrite
    break;
  }
}

void stepTowards(int &curSteps, int targetSteps, int &seqIndex, unsigned long &lastTime, unsigned long interval, int motorId)
{
  unsigned long now = millis();

  // If already at target - hold motor energized
  if (curSteps == targetSteps)
  {
    // Rewrite last position every 50ms to hold motor
    if (now - lastTime >= 10)
    {
      applyStepStateToMotor(motorId, seqIndex);
      lastTime = now;
    }
    return;
  }

  if (now - lastTime < interval)
    return;

  // Take MULTIPLE steps at once for smoothness
  int diff = abs(targetSteps - curSteps);
  int stepsToMove = 1;

  // Greater distance = more steps per iteration
  // Balanced for X27-168 motors: not too fast (skip steps), not too slow (lag)
  if (diff > 400)
    stepsToMove = 6; // Very very far - 6 steps (extreme events: restart/crash)
  else if (diff > 200)
    stepsToMove = 5; // Very far - 5 steps
  else if (diff > 120)
    stepsToMove = 4; // Far - 4 steps
  else if (diff > 75)
    stepsToMove = 3; // Medium-far - 3 steps
  else if (diff > 50)
    stepsToMove = 2; // Medium - 2 steps
  else if (diff > 20)
    stepsToMove = 2; // Close - 2 steps
  else
    stepsToMove = 1; // Very close - 1 step

  // Execute steps
  for (int i = 0; i < stepsToMove; i++)
  {
    if (curSteps == targetSteps)
      break; // Reached target

    if (curSteps < targetSteps)
    {
      seqIndex = (seqIndex + 1) % SEQ_LEN;
      curSteps++;
    }
    else
    {
      seqIndex = (seqIndex - 1 + SEQ_LEN) % SEQ_LEN;
      curSteps--;
    }
    applyStepStateToMotor(motorId, seqIndex);
    delayMicroseconds(700); // Small delay between steps
  }

  lastTime = now;
}

// --------------------------- LED CONTROL ---------------------------
void setLEDRange(int start, int count, CRGB color)
{
  for (int i = start; i < start + count && i < NUM_LEDS; i++)
  {
    leds[i] = color;
  }
}

void clearAllLEDs()
{
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void updateIndicatorLEDs()
{
  // FPS control: limit FastLED.show()
  unsigned long now = millis();
  if (now - lastLEDUpdate < LED_UPDATE_INTERVAL)
    return;
  lastLEDUpdate = now;
  clearAllLEDs();

  // Backlight FIRST (set background, then indicators will overwrite if needed)
  CRGB dimColor = currentBacklightColor;
  int brightnessValue = map(currentBrightnessPct, 0, 100, 0, 255); // Convert % to 0-255
  dimColor.nscale8(brightnessValue);

  // CRUISE CONTROL: Backlight groups 1 and 2 turn BLUE when cruise is active
  if (indCruise)
  {
    CRGB cruiseColor = CRGB(0, 100, 255); // Blue (same as high beam)
    cruiseColor.nscale8(brightnessValue);
    setLEDRange(LED_BACKLIGHT_1_START, LED_BACKLIGHT_1_COUNT, cruiseColor);
    setLEDRange(LED_BACKLIGHT_2_START, LED_BACKLIGHT_2_COUNT, cruiseColor);
  }
  else
  {
    setLEDRange(LED_BACKLIGHT_1_START, LED_BACKLIGHT_1_COUNT, dimColor);
    setLEDRange(LED_BACKLIGHT_2_START, LED_BACKLIGHT_2_COUNT, dimColor);
  }
  setLEDRange(LED_BACKLIGHT_3_START, LED_BACKLIGHT_3_COUNT, dimColor);

  // Turn signals
  if (indHazard || indLeftTurn)
  {
    leds[LED_LEFT_TURN] = CRGB(0, 255, 0); // Green
  }
  if (indHazard || indRightTurn)
  {
    leds[LED_RIGHT_TURN] = CRGB(0, 255, 0); // Green
  }

  // High beam (separate, blue)
  if (indHighBeam)
    leds[LED_HIGH_BEAM] = CRGB(0, 100, 255); // Blue

  // Under Tachometer indicators
  if (indHandbrake)
    leds[LED_HANDBRAKE] = CRGB(255, 255, 0); // Yellow (handbrake)
  if (indDoorsOpen)
    leds[LED_DOORS_OPEN] = CRGB(255, 255, 0); // Yellow
  if (indLowBeamOrParking)
    leds[LED_LOW_BEAM_OR_PARKING] = CRGB(0, 255, 0); // Green (low beam or parking lights)
  if (indABS)
    leds[LED_ABS] = CRGB(255, 255, 0); // Yellow
  if (indTC)
    leds[LED_TC] = CRGB(255, 255, 0); // Yellow
  // Under Speedometer indicators
  if (warningBatteryVolt)
    leds[LED_LOW_BATTERY] = CRGB(255, 0, 0); // Red
  if (indHazard)
    leds[LED_HAZARD_IND] = CRGB(255, 0, 0); // Red (hazard indicator)
  if (indLowOilPressure)
    leds[LED_LOW_OIL] = CRGB(255, 128, 0); // Orange
  if (indCheckEngine)
    leds[LED_CHECK_ENGINE] = CRGB(255, 0, 0); // Red
  if (indOverheat)
    leds[LED_OVERHEAT] = CRGB(255, 128, 0); // Orange

  // Center indicator
  if (indFuelLow)
    leds[LED_FUEL_LOW] = CRGB(255, 128, 0); // Orange

  // Shift lights (2 LEDs with 4-stage color and blinking at critical RPM)
  if (indShiftLight)
  {
    // If red (>= 96%), make it blink with fast strobe (75ms)
    if (shiftLightColor.r == 255 && shiftLightColor.g == 0 && shiftLightColor.b == 0)
    {
      // Fast blink red shift lights at critical RPM (13 Hz strobe)
      if (shiftLightBlinkState)
      {
        leds[LED_SHIFT_LIGHT_1] = shiftLightColor;
        leds[LED_SHIFT_LIGHT_2] = shiftLightColor;
      }
      else
      {
        // Keep backlight during blink OFF phase
        leds[LED_SHIFT_LIGHT_1] = CRGB::Black;
        leds[LED_SHIFT_LIGHT_2] = CRGB::Black;
      }
    }
    else
    {
      // Solid green/yellow/orange for warning zones
      leds[LED_SHIFT_LIGHT_1] = shiftLightColor;
      leds[LED_SHIFT_LIGHT_2] = shiftLightColor;
    }
  }
  else
  {
    // When shift light is OFF, show backlight (same as other backlight LEDs)
    leds[LED_SHIFT_LIGHT_1] = dimColor;
    leds[LED_SHIFT_LIGHT_2] = dimColor;
  }

  FastLED.show();
}

void handleTurnSignalClick()
{
  // SimHub sends turn signals with blinking (0/1 alternates)
  // We detect state changes and play click sound (relay simulation)
  bool currentState = (indHazard || indLeftTurn || indRightTurn);

  // Click when signal goes from 0 to 1 (rising edge)
  if (currentState && !lastTurnSignalState)
  {
    playClick(true); // ON click (relay pull-in)
  }
  // Click when signal goes from 1 to 0 (falling edge) - relay release
  else if (!currentState && lastTurnSignalState)
  {
    playClick(false); // OFF click (relay release)
  }

  lastTurnSignalState = currentState;
}

void handleShiftLightBlink()
{
  unsigned long now = millis();
  if (now - shiftLightLastBlink >= SHIFT_LIGHT_INTERVAL)
  {
    shiftLightLastBlink = now;
    shiftLightBlinkState = !shiftLightBlinkState;
  }
}

// --------------------------- COMPUTED INDICATORS ---------------------------
void computeIndicators()
{
  // HAZARD LIGHTS: Software fallback (if SimHub didn't send HAZARD flag)
  // If both turn signals are active, it's hazard lights
  if (indLeftTurn && indRightTurn)
  {
    indHazard = true;
  }

  // ENGINE_RUNNING: RPM > 0
  indEngineRunning = (curRpm > 0);

  // CHECK_ENGINE: overheat OR low oil pressure OR engine damage
  indCheckEngine = (curWaterTemp > TEMP_MAX - 10.0f) || (curOilPress > 0 && curOilPress < 0.5f) || (!indEngineRunning) || warningAirPress || warningOilPress || warningBatteryVolt;

  // FUEL_LOW: less than 15%
  indFuelLow = (curFuel >= 0 && curFuel <= 15.0f);

  // LOW_OIL_PRESSURE: pressure near zero
  indLowOilPressure = (curOilPress >= 0 && curOilPress < 0.3f) || warningOilPress;

  // OVERHEAT: water temp above max
  indOverheat = (curWaterTemp > TEMP_MAX - 5.0f) || warningWaterTemp;

  // DOORS_OPEN: speed 0-2 km/h (no ignition tracking needed)
  bool lowSpeed = (curKmh >= 0.0f && curKmh <= 2.0f);
  indDoorsOpen = lowSpeed;

  // SHIFT_LIGHT: Single LED with 4-stage color progression based on RPM percentage
  if (curRpm > 0 && curMaxRPM > 0)
  {
    float rpmPercent = (curRpm / curMaxRPM) * 100.0f;

    if (rpmPercent < RPM_ZONE_GREEN_START)
    {
      // Below green zone: LED off
      indShiftLight = false;
      shiftLightColor = CRGB::Black;
    }
    else if (rpmPercent < RPM_ZONE_YELLOW_START)
    {
      // Green zone: Early warning
      indShiftLight = true;
      shiftLightColor = CRGB(0, 255, 0); // Green
    }
    else if (rpmPercent < RPM_ZONE_ORANGE_START)
    {
      // Yellow zone: Approaching shift point
      indShiftLight = true;
      shiftLightColor = CRGB(255, 255, 0); // Yellow
    }
    else if (rpmPercent < RPM_ZONE_RED_START)
    {
      // Orange zone: Shift now!
      indShiftLight = true;
      shiftLightColor = CRGB(255, 128, 0); // Orange
    }
    else
    {
      // Red zone: Critical RPM (will blink in updateIndicatorLEDs)
      indShiftLight = true;
      shiftLightColor = CRGB(255, 0, 0); // Red
    }
  }
  else
  {
    indShiftLight = false;
    shiftLightColor = CRGB::Black;
  }
}

// --------------------------- LIGHT INDICATORS FALLBACK ---------------------------
void applyLightIndicatorsFallback()
{

  // No light data at all - use engine running as indicator
  if (!parkingSupportedByGame && !lowBeamSupportedByGame)
  {
    // If engine is running (RPM > 0), assume lights are on
    if (indEngineRunning)
    {
      indLowBeamOrParking = true;
    }
    else
    {
      indLowBeamOrParking = false;
    }
    indHighBeam = false; // Never assume high beam
    return;
  }

  // Edge cases: Parking supported but not Low/High - use parking as-is
  // (rare, but possible in some games like truck simulators)
}

// --------------------------- SIMHUB PARSING ---------------------------
void parseKeyValueLine(const String &line)
{
  int pos = 0;
  while (pos < (int)line.length())
  {
    int sc = line.indexOf(';', pos);
    String token = (sc == -1) ? line.substring(pos) : line.substring(pos, sc);
    pos = (sc == -1) ? line.length() : sc + 1;
    token.trim();
    if (token.length() == 0)
      continue;
    int c = token.indexOf(':');
    if (c == -1)
      continue;
    String key = token.substring(0, c);
    key.trim();
    String val = token.substring(c + 1);
    val.trim();

    // Speed
    if (key.equalsIgnoreCase("KMH"))
    {
      if (val.length() == 0)
        curKmh = -1.0f;
      else
      {
        float nv = val.toFloat();
        // Capture a TRUE game zero (restart/full stop) BEFORE the deadzone, so the synced
        // force-to-zero sweep fires only on a real zero - not on a deadzoned residual/glitch.
        bool gameReportsZero = (nv == 0.0f);
        // Deadzone: a physical needle can't meaningfully show <2 km/h. Snapping tiny
        // crash/settle residuals to exact 0 lets the big-drop bypass drive the needle home.
        if (nv < SPEED_ZERO_SNAP)
          nv = 0.0f;
        if (inRangeFloat(nv, SPEED_MIN, SPEED_MAX_POSS) && jumpOK(nv, lastGoodKmh, MAX_JUMP_SPEED))
        {
          // EXTREME EVENT: only a real game zero (restart/full stop) triggers the synced sweep.
          // Crash residuals (<2 km/h) reach zero via the big-drop bypass in the else branch.
          if (lastGoodKmh >= LAST_GOOD_KMH && gameReportsZero)
          {
            forceSpeedToZero = true; // Force needle to zero like welcome animation
            curKmh = 0.0f;
          }
          else
          {
            // EMA only for small gradual changes below threshold (damps jitter, keeps inertia).
            // A large sudden drop is a real event (crash/hard brake) - apply directly, no lag.
            if (nv < curKmh && nv < LOW_SPEED_THRESHOLD && (curKmh - nv) <= BIG_DROP_SPEED)
            {
              curKmh = smooth(curKmh, nv, SMOOTH_ALPHA_LOW);
            }
            else
            {
              curKmh = nv; // Direct assignment for precision
            }
          }
          lastGoodKmh = nv;
        }
        else if (!inRangeFloat(nv, SPEED_MIN, SPEED_MAX_POSS))
          curKmh = -1.0f;
      }
    }
    // RPM
    else if (key.equalsIgnoreCase("RPM"))
    {
      if (val.length() == 0)
        curRpm = -1.0f;
      else
      {
        float nv = val.toFloat();
        if (inRangeFloat(nv, RPM_MIN, RPM_MAX_POSS) && jumpOK(nv, lastGoodRpm, MAX_JUMP_RPM))
        {
          // EXTREME EVENT: RPM drop from 3000+ to 0 (restart/fatal crash)
          if (lastGoodRpm >= 3000.0f && nv == 0.0f)
          {
            forceRpmToZero = true; // Force needle to zero like welcome animation
            curRpm = 0.0f;
          }
          else
          {
            // Apply EMA only when decelerating below LOW_RPM_THRESHOLD (realistic inertia)
            if (nv < curRpm && nv < LOW_RPM_THRESHOLD)
            {
              curRpm = smooth(curRpm, nv, SMOOTH_ALPHA_LOW);
            }
            else
            {
              curRpm = nv; // Direct assignment for precision
            }
          }
          lastGoodRpm = nv;
        }
        else if (!inRangeFloat(nv, RPM_MIN, RPM_MAX_POSS))
          curRpm = -1.0f;
      }
    }
    // Fuel %
    else if (key.equalsIgnoreCase("FUEL"))
    {
      if (val.length() == 0)
        curFuel = -1.0f;
      else
      {
        float nv = val.toFloat();
        if (inRangeFloat(nv, FUEL_MIN, FUEL_MAX))
        {
          curFuel = nv;
          // Mark as supported if non-zero value received at least once
          if (nv > 0.1f)
            fuelSupportedByGame = true;
        }
        else
          curFuel = -1.0f;
      }
    }
    // Water Temp
    else if (key.equalsIgnoreCase("WATER"))
    {
      if (val.length() == 0)
        curWaterTemp = -1.0f;
      else
      {
        float nv = val.toFloat();
        if (inRangeFloat(nv, TEMP_MIN, TEMP_MAX) && jumpOK(nv, lastGoodWaterTemp, MAX_JUMP_TEMP))
        {
          curWaterTemp = nv;
          lastGoodWaterTemp = nv;
          // Mark as supported if non-zero value received at least once
          if (nv > 0.1f)
            waterTempSupportedByGame = true;
        }
        else if (!inRangeFloat(nv, TEMP_MIN, TEMP_MAX))
          curWaterTemp = -1.0f;
      }
    }
    // Oil Pressure
    else if (key.equalsIgnoreCase("OILPRESS"))
    {
      if (val.length() == 0)
        curOilPress = -1.0f;
      else
      {
        float nv = val.toFloat();
        if (inRangeFloat(nv, OILP_MIN, OILP_MAX) && jumpOK(nv, lastGoodOilPress, MAX_JUMP_OILP))
        {
          curOilPress = nv;
          lastGoodOilPress = nv;
        }
        else if (!inRangeFloat(nv, OILP_MIN, OILP_MAX))
          curOilPress = -1.0f;
      }
    }
    // Dynamic shift lights from SimHub
    else if (key.equalsIgnoreCase("MAXRPM"))
    {
      float nv = val.toFloat();
      if (nv > 0 && nv < 20000.0f)
        curMaxRPM = nv; // Update max RPM for current car
    }
    // Indicators from SimHub (optional, boolean: 0 or 1)
    else if (key.equalsIgnoreCase("LEFTTURN"))
      indLeftTurn = val.toInt();
    else if (key.equalsIgnoreCase("RIGHTTURN"))
      indRightTurn = val.toInt();
    else if (key.equalsIgnoreCase("PARKLIGHT"))
    {
      indLowBeamOrParking = val.toInt();
      if (indLowBeamOrParking == 1)
        parkingSupportedByGame = true; // Mark as supported (once received = forever supported)
    }
    else if (key.equalsIgnoreCase("LOWBEAM"))
    {
      indLowBeamOrParking = val.toInt();
      if (indLowBeamOrParking == 1)
        lowBeamSupportedByGame = true; // Mark as supported
    }
    else if (key.equalsIgnoreCase("HIGHBEAM"))
    {
      indHighBeam = val.toInt();
      if (indHighBeam == 1)
        highBeamSupportedByGame = true; // Mark as supported
    }
    else if (key.equalsIgnoreCase("CHECKENGINE"))
      indCheckEngine = val.toInt();
    else if (key.equalsIgnoreCase("HANDBRAKE"))
      indHandbrake = val.toInt();
    else if (key.equalsIgnoreCase("ABS"))
      indABS = val.toInt();
    else if (key.equalsIgnoreCase("TC"))
      indTC = val.toInt();
    else if (key.equalsIgnoreCase("CRUISE"))
      indCruise = val.toInt();
    else if (key.equalsIgnoreCase("WARNING_AIR_PRESS"))
    {
      warningAirPress = val.toInt();
    }
    else if (key.equalsIgnoreCase("WARNING_OIL_PRESS"))
    {
      warningOilPress = val.toInt();
    }
    else if (key.equalsIgnoreCase("WARNING_BATTERY_VOLT"))
    {
      warningBatteryVolt = val.toInt();
    }
    else if (key.equalsIgnoreCase("WARNING_WATER_TEMP"))
    {
      warningWaterTemp = val.toInt();
    }
  }
}

void readSimHubSerial()
{
  if (!serialReady)
  {
    // Discard everything received before ready
    while (Serial.available() > 0)
    {
      Serial.read();
    }
    return;
  }

  // TIGHT LOOP: read ALL available bytes without delays
  // This is critical to prevent 64-byte RX buffer overflow on Mega
  while (Serial.available())
  {
    char c = (char)Serial.read();

    // Skip control characters (except \n)
    if (c < 32 && c != '\n')
    {
      continue;
    }

    // End of line - parse
    if (c == '\n')
    {
      if (serialBufIdx > 0)
      {
        // Null-terminate string
        serialBuf[serialBufIdx] = '\0';

        // Parse only if string is long enough
        if (serialBufIdx > 10)
        {
          // Reset all indicator variables before parsing new packet
          curMaxRPM = 8000.0f;
          indLeftTurn = false;
          indRightTurn = false;
          indHazard = false;
          indLowBeamOrParking = false;
          indHighBeam = false;
          indHandbrake = false;
          indABS = false;
          indTC = false;
          indCruise = false;
          indCheckEngine = false;
          warningAirPress = false;
          warningOilPress = false;
          warningBatteryVolt = false;
          warningWaterTemp = false;

          parseKeyValueLine(String(serialBuf)); // Convert char* to String
          lastSimhubPacket = millis();

          // Decrement error counter on successful reception
          if (serial_overflow_count > 0)
          {
            serial_overflow_count--;
          }
        }

        // Reset buffer for next line
        serialBufIdx = 0;
      }
    }
    // Add character to buffer
    else if (c >= 32 && c < 127)
    {
      if (serialBufIdx < SERIAL_MAX_BUFFER - 1)
      {
        serialBuf[serialBufIdx++] = c;
      }
      else
      {
        // Overflow - reset buffer
        serialBufIdx = 0;
        serial_overflow_count++;

        // Critical overflow - reboot
        if (serial_overflow_count > 20)
        {
          Serial.end();
          wdt_enable(WDTO_15MS);
          while (1)
          {
          }
        }
      }
    }
  }
}

void updateStepsDirect(int dSpeed, int dRpm, int dFuel, int dTemp)
{
  // Update step sequence indices (cyclic 0-7)
  if (dSpeed != 0)
    seqIndexSpeed = (seqIndexSpeed + dSpeed + SEQ_LEN) % SEQ_LEN;
  if (dRpm != 0)
    seqIndexRpm = (seqIndexRpm + dRpm + SEQ_LEN) % SEQ_LEN;
  if (dFuel != 0)
    seqIndexFuel = (seqIndexFuel + dFuel + SEQ_LEN) % SEQ_LEN;
  if (dTemp != 0)
    seqIndexTemp = (seqIndexTemp + dTemp + SEQ_LEN) % SEQ_LEN;

  // Send signals to drivers
  if (dSpeed != 0)
    applyStepStateToMotor(0, seqIndexSpeed);
  if (dRpm != 0)
    applyStepStateToMotor(1, seqIndexRpm);
  if (dFuel != 0)
    applyStepStateToMotor(2, seqIndexFuel);
  if (dTemp != 0)
    applyStepStateToMotor(3, seqIndexTemp);
}

// --------------------------- WELCOME SEQUENCE ---------------------------
void welcomeSequence()
{
  // 1. FORCED ZERO CALIBRATION
  // Rotate motors backward by more than 360 degrees
  // To ensure all needles hit their physical stops
  for (int i = 0; i < (STEPS_PER_REV); i++)
  {
    updateStepsDirect(-1, -1, -1, -1);
    delayMicroseconds(500);

    // Periodically reset watchdog
    if (i % 200 == 0)
    {
      wdt_reset();
    }
  }
  wdt_reset();

  curSpeedSteps = 0;
  curRpmSteps = 0;
  curFuelSteps = 0;
  curTempSteps = 0;

  // All 4 needles sweep from min to max in parallel
  int speedSteps = SPEED_MAX_STEPS - SPEED_MIN_STEPS;
  int rpmSteps = RPM_MAX_STEPS - RPM_MIN_STEPS;
  int fuelSteps = FUEL_MAX_STEPS - FUEL_MIN_STEPS;
  int tempSteps = TEMP_MAX_STEPS - TEMP_MIN_STEPS;
  int maxSteps = max(max(speedSteps, rpmSteps), max(fuelSteps, tempSteps));

  // Turn on backlight immediately (stays on permanently) - both groups
  CRGB dimBacklight = currentBacklightColor;
  int brightnessValue = map(currentBrightnessPct, 0, 100, 0, 255);
  dimBacklight.nscale8(brightnessValue);
  setLEDRange(LED_BACKLIGHT_1_START, LED_BACKLIGHT_1_COUNT, dimBacklight);
  setLEDRange(LED_BACKLIGHT_2_START, LED_BACKLIGHT_2_COUNT, dimBacklight);
  setLEDRange(LED_BACKLIGHT_3_START, LED_BACKLIGHT_3_COUNT, dimBacklight);

  // Turn on ALL indicators (simulate full dashboard test)
  // Indicators are: 0-2, 7-14, 17-21
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Skip backlight groups (3-6, 15-16, 22-27)
    if ((i >= 3 && i <= 6) || (i >= 12 && i <= 13) || (i >= 22 && i <= 27))
    {
      continue; // Already set backlight
    }
    leds[i] = dimBacklight;
  }
  FastLED.show();

  // Sweep needles up
  for (int i = 0; i <= maxSteps; i++)
  {
    if (i <= speedSteps)
    {
      curSpeedSteps = SPEED_MIN_STEPS + i;
      seqIndexSpeed = (seqIndexSpeed + 1) % SEQ_LEN;
      applyStepStateToMotor(0, seqIndexSpeed);
    }
    if (i <= rpmSteps)
    {
      curRpmSteps = RPM_MIN_STEPS + i;
      seqIndexRpm = (seqIndexRpm + 1) % SEQ_LEN;
      applyStepStateToMotor(1, seqIndexRpm);
    }
    if (i <= fuelSteps)
    {
      curFuelSteps = FUEL_MIN_STEPS + i;
      seqIndexFuel = (seqIndexFuel + 1) % SEQ_LEN;
      applyStepStateToMotor(2, seqIndexFuel);
    }
    if (i <= tempSteps)
    {
      curTempSteps = TEMP_MIN_STEPS + i;
      seqIndexTemp = (seqIndexTemp + 1) % SEQ_LEN;
      applyStepStateToMotor(3, seqIndexTemp);
    }
    delay(1);
  }
  delay(50); // Pause at maximum

  // Sweep needles down
  for (int i = maxSteps; i >= 0; i--)
  {
    if (i <= speedSteps)
    {
      curSpeedSteps = SPEED_MIN_STEPS + i;
      seqIndexSpeed = (seqIndexSpeed - 1 + SEQ_LEN) % SEQ_LEN;
      applyStepStateToMotor(0, seqIndexSpeed);
    }
    if (i <= rpmSteps)
    {
      curRpmSteps = RPM_MIN_STEPS + i;
      seqIndexRpm = (seqIndexRpm - 1 + SEQ_LEN) % SEQ_LEN;
      applyStepStateToMotor(1, seqIndexRpm);
    }
    if (i <= fuelSteps)
    {
      curFuelSteps = FUEL_MIN_STEPS + i;
      seqIndexFuel = (seqIndexFuel - 1 + SEQ_LEN) % SEQ_LEN;
      applyStepStateToMotor(2, seqIndexFuel);
    }
    if (i <= tempSteps)
    {
      curTempSteps = TEMP_MIN_STEPS + i;
      seqIndexTemp = (seqIndexTemp - 1 + SEQ_LEN) % SEQ_LEN;
      applyStepStateToMotor(3, seqIndexTemp);
    }
    delay(1);
  }
  for (int j = 0; j < 5; j++)
  {
    wdt_reset();
    updateStepsDirect(-1, -1, -1, -1);
    delay(3);
  }

  // Turn OFF all indicators (except backlight)
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Skip shift lights (15-16, idle as backlight) and backlight groups 1 & 3 (3-6, 22-27).
    // Backlight group 2 (12-13) is cleared here and re-lit by setLEDRange below.
    if ((i >= 3 && i <= 6) || (i >= 15 && i <= 16) || (i >= 22 && i <= 27))
    {
      continue; // Keep backlight on
    }
    leds[i] = CRGB::Black; // Turn off indicators
  }
  // Backlight stays on (use current brightness) - both groups
  CRGB dimBacklight2 = currentBacklightColor;
  int brightnessValue2 = map(currentBrightnessPct, 0, 100, 0, 255);
  dimBacklight2.nscale8(brightnessValue2);
  setLEDRange(LED_BACKLIGHT_1_START, LED_BACKLIGHT_1_COUNT, dimBacklight2);
  setLEDRange(LED_BACKLIGHT_2_START, LED_BACKLIGHT_2_COUNT, dimBacklight2);
  setLEDRange(LED_BACKLIGHT_3_START, LED_BACKLIGHT_3_COUNT, dimBacklight2);
  FastLED.show();

  // Move needles to default "ready" positions (before SimHub connects)
  // Fuel: 75% (realistic race start fuel level)
  float fuelFrac = DEFAULT_FUEL_LEVEL / 100.0f;
  int defaultFuelSteps = FUEL_MIN_STEPS + (int)(fuelFrac * (FUEL_MAX_STEPS - FUEL_MIN_STEPS));

  // Temperature: 90°C (normal operating temperature)
  float tempFrac = (DEFAULT_WATER_TEMP - 0.0f) / 120.0f;
  int defaultTempSteps = TEMP_MIN_STEPS + (int)(tempFrac * (TEMP_MAX_STEPS - TEMP_MIN_STEPS));

  // De-energize motors
  analogWrite(TB1_PWMA, 0);
  analogWrite(TB1_PWMB, 0);
  analogWrite(TB2_PWMA, 0);
  analogWrite(TB2_PWMB, 0);
  analogWrite(TB3_PWMA, 0);
  analogWrite(TB3_PWMB, 0);
  analogWrite(TB4_PWMA, 0);
  analogWrite(TB4_PWMB, 0);

  // ==========================================
  // CRITICAL: Final buffer clear and enable reception
  // ==========================================

  // Fast clear WITHOUT blocking loops and delay!
  while (Serial.available() > 0)
  {
    Serial.read();
  }
  serialBufIdx = 0;
  serialBuf[0] = '\0';

  // ENABLE data reception
  serialReady = true;

  Serial.println("========================================");
  Serial.println("INFO: Welcome animation COMPLETE");
  Serial.println("========================================");
}

void doResetSequence()
{
  currentBacklightColor = colorPresets[0];
  welcomeSequence();
  // Clear data
  curKmh = -1.0f;
  curRpm = -1.0f;
  curFuel = -1.0f;
  curWaterTemp = -1.0f;
  curOilPress = -1.0f;
  indLeftTurn = indRightTurn = indHazard = false;
  indLowBeamOrParking = false;
  indCheckEngine = indLowOilPressure = false;
  indHandbrake = indOverheat = indABS = false;
  indTC = false;
  indCruise = false;
  indEngineRunning = indFuelLow = indDoorsOpen = false;
  indShiftLight = false; // Single shift light indicator
  warningAirPress = warningBatteryVolt = warningOilPress = warningWaterTemp = false;

  // Reset dynamic shift light thresholds to defaults
  curMaxRPM = 8000.0f;

  // Reset light support detection
  parkingSupportedByGame = false;
  lowBeamSupportedByGame = false;
  highBeamSupportedByGame = false;

  // Reset telemetry support detection
  fuelSupportedByGame = false;
  waterTempSupportedByGame = false;

  lastSimhubPacket = 0;
  sysState = STATE_WAIT_SIMHUB;
}

// --------------------------- BUTTONS ---------------------------
void readBrightnessPotentiometer()
{
  unsigned long now = millis();
  if (now - lastBrightnessRead < BRIGHTNESS_READ_INTERVAL)
    return;
  lastBrightnessRead = now;

  // Read potentiometer value (0-1023)
  int rawValue = analogRead(BRIGHTNESS_POT_PIN);

  // Map to brightness percentage (10-100%)
  currentBrightnessPct = map(rawValue, 0, 1023, BRIGHTNESS_MIN_PERCENT, BRIGHTNESS_MAX_PERCENT);
  currentBrightnessPct = constrain(currentBrightnessPct, BRIGHTNESS_MIN_PERCENT, BRIGHTNESS_MAX_PERCENT);
}

void processButtons()
{
  // RESET button
  bool rawReset = digitalRead(RESET_BTN_PIN);
  if (rawReset == LOW && (millis() - reset_last > BTN_DEBOUNCE_MS))
  {
    reset_last = millis();
    Serial.println("RESET button pressed");
    doResetSequence();
  }

  // COLOR button (cycle backlight color)
  bool rawColor = digitalRead(COLOR_BTN_PIN);
  if (rawColor == LOW && (millis() - color_last > BTN_DEBOUNCE_MS))
  {
    color_last = millis();
    colorPresetIndex = (colorPresetIndex + 1) % NUM_COLOR_PRESETS;
    currentBacklightColor = colorPresets[colorPresetIndex];
    Serial.print("COLOR changed to preset #");
    Serial.print(colorPresetIndex);
    Serial.print(" RGB: ");
    Serial.print(currentBacklightColor.r);
    Serial.print(", ");
    Serial.print(currentBacklightColor.g);
    Serial.print(", ");
    Serial.println(currentBacklightColor.b);
  }
}

// --------------------------- SETUP ---------------------------
void setup()
{
  Serial.begin(115200);
  delay(1000);
  wdt_disable();
  serialReady = false;

  while (Serial.available() > 0)
  {
    Serial.read();
  }
  serialBufIdx = 0;
  serialBuf[0] = '\0';

  Serial.println("========================================");
  Serial.println("Mitsubishi Galant Dashboard - BOOT");
  Serial.println("========================================");
  Serial.println("INFO: Serial initialized, starting welcome animation...");
  Serial.println("========================================");

  // TB6612 pins - explicitly set as OUTPUT (best practice)
  pinMode(TB1_AIN1, OUTPUT);
  pinMode(TB1_AIN2, OUTPUT);
  pinMode(TB1_BIN1, OUTPUT);
  pinMode(TB1_BIN2, OUTPUT);
  pinMode(TB1_PWMA, OUTPUT);
  pinMode(TB1_PWMB, OUTPUT);

  pinMode(TB2_AIN1, OUTPUT);
  pinMode(TB2_AIN2, OUTPUT);
  pinMode(TB2_BIN1, OUTPUT);
  pinMode(TB2_BIN2, OUTPUT);
  pinMode(TB2_PWMA, OUTPUT);
  pinMode(TB2_PWMB, OUTPUT);

  pinMode(TB3_AIN1, OUTPUT);
  pinMode(TB3_AIN2, OUTPUT);
  pinMode(TB3_BIN1, OUTPUT);
  pinMode(TB3_BIN2, OUTPUT);
  pinMode(TB3_PWMA, OUTPUT);
  pinMode(TB3_PWMB, OUTPUT);

  pinMode(TB4_AIN1, OUTPUT);
  pinMode(TB4_AIN2, OUTPUT);
  pinMode(TB4_BIN1, OUTPUT);
  pinMode(TB4_BIN2, OUTPUT);
  pinMode(TB4_PWMA, OUTPUT);
  pinMode(TB4_PWMB, OUTPUT);

  pinMode(TB_STBY, OUTPUT);
  digitalWrite(TB_STBY, HIGH); // Enable all TB6612 drivers

  // Speaker for turn signal clicks
  pinMode(SPEAKER_PIN, OUTPUT);

  // Buttons
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);
  pinMode(COLOR_BTN_PIN, INPUT_PULLUP);

  // Potentiometer (analog input, no pinMode needed)
  // Read initial brightness value
  int initialRaw = analogRead(BRIGHTNESS_POT_PIN);
  currentBrightnessPct = map(initialRaw, 0, 1023, BRIGHTNESS_MIN_PERCENT, BRIGHTNESS_MAX_PERCENT);

  // WS2812B
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  clearAllLEDs();
  FastLED.show();

  // Diagnostic: Print default backlight color
  Serial.print("[BACKLIGHT] Default color RGB: ");
  Serial.print(currentBacklightColor.r);
  Serial.print(", ");
  Serial.print(currentBacklightColor.g);
  Serial.print(", ");
  Serial.println(currentBacklightColor.b);

  // Welcome animation
  welcomeSequence();

  sysState = STATE_WAIT_SIMHUB;
  lastSimhubPacket = millis();

  wdt_enable(WDTO_4S);
  lastWdtReset = millis();
  Serial.println("INFO: Watchdog enabled (4s timeout)");
  Serial.println("INFO: Waiting for SimHub connection...");
}

// --------------------------- MAIN LOOP ---------------------------
void loop()
{
  // Watchdog reset
  if (millis() - lastWdtReset >= WDT_RESET_INTERVAL)
  {
    wdt_reset();
    lastWdtReset = millis();
  }

  // Free-RAM diagnostic (every 10s) - watch for a steady decline over a session
  if (millis() - lastRamReport >= RAM_REPORT_INTERVAL)
  {
    lastRamReport = millis();
    Serial.print(F("[RAM] free bytes: "));
    Serial.println(freeMemory());
  }

  // Read SimHub data
  readSimHubSerial();

  // Read brightness potentiometer
  readBrightnessPotentiometer();

  // Process buttons
  processButtons();

  // Check SimHub connection
  bool simhubAlive = (millis() - lastSimhubPacket) < SIMHUB_TIMEOUT_MS;
  if (sysState == STATE_WAIT_SIMHUB && simhubAlive)
  {
    sysState = STATE_RUNNING;
    Serial.println("INFO: SimHub connected!");
  }

  // Compute derived indicators
  computeIndicators();

  // Apply fallback logic for light indicators if not supported by game
  applyLightIndicatorsFallback();

  // Update turn signal blink
  handleTurnSignalClick();

  // Update shift light blink (fast strobe)
  handleShiftLightBlink();

  // Update LED indicators
  updateIndicatorLEDs();

  // EXTREME EVENT: Force needles to zero (PARALLEL, like welcome animation)
  // RPM: 3000+ → 0, Speed: 60+ → 0 - move BOTH at the same time
  bool extremeEventActive = forceRpmToZero || forceSpeedToZero;
  if (extremeEventActive)
  {
    // Move RPM to zero
    if (forceRpmToZero && curRpmSteps > RPM_MIN_STEPS)
    {
      seqIndexRpm = (seqIndexRpm - 1 + SEQ_LEN) % SEQ_LEN;
      curRpmSteps--;
      applyStepStateToMotor(1, seqIndexRpm);
      if (curRpmSteps <= RPM_MIN_STEPS)
        forceRpmToZero = false; // Done
    }
    // Move Speed to zero
    if (forceSpeedToZero && curSpeedSteps > SPEED_MIN_STEPS)
    {
      seqIndexSpeed = (seqIndexSpeed - 1 + SEQ_LEN) % SEQ_LEN;
      curSpeedSteps--;
      applyStepStateToMotor(0, seqIndexSpeed);
      if (curSpeedSteps <= SPEED_MIN_STEPS)
        forceSpeedToZero = false; // Done
    }
    delay(1); // 1 step every 1 ms (like welcome animation)
    return;   // Skip normal motor updates
  }

  // Update stepper motors (freeze on disconnect)
  bool freezeNeedles = !simhubAlive;

  // Speedometer
  int targetSpeedSteps = SPEED_MIN_STEPS;
  if (!freezeNeedles && curKmh >= 0.0f)
  {
    // Non-linear calibration: 0-20 km/h compressed into small zone
    // Physical gauge: 0-20 is tiny, 20-220 is stretched across rest of scale
    float frac;
    if (curKmh <= 20.0f)
    {
      // 0-20 km/h → 0-10% of needle travel (very compressed)
      frac = (curKmh / 20.0f) * 0.05f;
    }
    else
    {
      // 20-220 km/h → 10-100% of needle travel (normal spacing)
      float remaining = constrain(curKmh - 20.0f, 0.0f, 200.0f); // 200 = 220-20
      frac = 0.05f + (remaining / 200.0f) * 0.95f;
    }
    targetSpeedSteps = SPEED_MIN_STEPS + (int)(frac * (SPEED_MAX_STEPS - SPEED_MIN_STEPS));
  }
  int diffS = abs(targetSpeedSteps - curSpeedSteps);
  // Ultra-fast mode for extreme events (6 steps @ 2ms = balanced for X27-168)
  unsigned long intervalS;
  if (diffS > 300)
    intervalS = 2; // Ultra-fast: 2 ms (extreme events, balanced)
  else if (diffS > 50)
    intervalS = STEP_INTERVAL_FAST; // Fast: 3 ms
  else
    intervalS = STEP_INTERVAL_SLOW; // Slow: 5 ms
  stepTowards(curSpeedSteps, targetSpeedSteps, seqIndexSpeed, lastStepTimeSpeed, intervalS, 0);

  // Tachometer (always use physical gauge scale 0-8000, DO NOT scale to car's MaxRPM)
  int targetRpmSteps = RPM_MIN_STEPS;
  if (!freezeNeedles && curRpm >= 0.0f)
  {
    // Use GAUGE_MAX_RPM (8000) - the physical gauge scale, NOT curMaxRPM
    // This ensures needle position always matches physical markings on Galant gauge
    // Examples:
    //   RPM=2000 → 2000/8000 = 25% → needle shows 2000 on physical scale ✓
    //   RPM=6000 → 6000/8000 = 75% → needle shows 6000 on physical scale ✓
    //   RPM=12000 → 12000/8000 = 1.5 → constrain to 1.0 → needle maxed at 8000 ✓
    float frac = constrain(curRpm / (float)GAUGE_MAX_RPM, 0.0f, 1.0f);
    targetRpmSteps = RPM_MIN_STEPS + (int)(frac * (RPM_MAX_STEPS - RPM_MIN_STEPS));
  }
  int diffR = abs(targetRpmSteps - curRpmSteps);
  // Ultra-fast mode for extreme events (6 steps @ 2ms = balanced for X27-168)
  unsigned long intervalR;
  if (diffR > 300)
    intervalR = 2; // Ultra-fast: 2 ms (extreme events, balanced)
  else if (diffR > 40)
    intervalR = STEP_INTERVAL_FAST; // Fast: 3 ms
  else
    intervalR = STEP_INTERVAL_SLOW; // Slow: 5 ms
  stepTowards(curRpmSteps, targetRpmSteps, seqIndexRpm, lastStepTimeRpm, intervalR, 1);

  // Fuel Level
  int targetFuelSteps = FUEL_MIN_STEPS;
  if (!freezeNeedles)
  {
    // Use default only if game doesn't support fuel telemetry
    float fuelValue = curFuel;
    if (curFuel < 0.0f && !fuelSupportedByGame)
      fuelValue = DEFAULT_FUEL_LEVEL;
    else if (curFuel < 0.0f)
      fuelValue = 0.0f; // Game supports fuel but no data yet - show empty
    float frac = constrain(fuelValue / 100.0f, 0.0f, 1.0f);
    targetFuelSteps = FUEL_MIN_STEPS + (int)(frac * (FUEL_MAX_STEPS - FUEL_MIN_STEPS));
  }
  int diffF = abs(targetFuelSteps - curFuelSteps);
  // Ultra-fast mode for extreme events (6 steps @ 2ms = balanced for X27-168)
  unsigned long intervalF;
  if (diffF > 200)
    intervalF = 2; // Ultra-fast: 2 ms (extreme events, balanced)
  else if (diffF > 30)
    intervalF = STEP_INTERVAL_FAST; // Fast: 3 ms
  else
    intervalF = STEP_INTERVAL_SLOW; // Slow: 5 ms
  stepTowards(curFuelSteps, targetFuelSteps, seqIndexFuel, lastStepTimeFuel, intervalF, 2);

  // Coolant Temperature
  int targetTempSteps = TEMP_MIN_STEPS;
  if (!freezeNeedles)
  {
    // Use default only if game doesn't support water temp telemetry
    float tempValue = curWaterTemp;
    if (curWaterTemp < 0.0f && !waterTempSupportedByGame)
      tempValue = DEFAULT_WATER_TEMP;
    else if (curWaterTemp < 0.0f)
      tempValue = 0.0f; // Game supports temp but no data yet - show minimum
    // Map temperature: 0°C = TEMP_MIN_STEPS, 120°C = TEMP_MAX_STEPS
    float frac = constrain((tempValue - 0.0f) / 120.0f, 0.0f, 1.0f);
    targetTempSteps = TEMP_MIN_STEPS + (int)(frac * (TEMP_MAX_STEPS - TEMP_MIN_STEPS));
  }
  int diffT = abs(targetTempSteps - curTempSteps);
  // Ultra-fast mode for extreme events (6 steps @ 2ms = balanced for X27-168)
  unsigned long intervalT;
  if (diffT > 150)
    intervalT = 2; // Ultra-fast: 2 ms (extreme events, balanced)
  else if (diffT > 30)
    intervalT = STEP_INTERVAL_FAST; // Fast: 3 ms
  else
    intervalT = STEP_INTERVAL_SLOW; // Slow: 5 ms
  stepTowards(curTempSteps, targetTempSteps, seqIndexTemp, lastStepTimeTemp, intervalT, 3);
}
