/*
  Digital VFD Sim-Dashboard

  Hardware:
  - ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)
  - 1x TFT SPI 3.5" (480x320) ST7796
  - SimHub integration via Serial (USB CDC)

  Wiring:
    MOSI: GPIO 11  →  Display SDI
    SCLK: GPIO 12  →  Display SCK
    DC:   GPIO  9  →  Display DC
    RST:  GPIO  8  →  Display RESET
    CS:   GPIO 10  →  Display CS
    LED:  GPIO 13  →  Display LED (PWM)
    VCC:  5V, GND: GND

  Display Layout (480x320):
  ┌─────────────────────────────────────────────────────────────┐
  │ [<-][ABS][LOW][HAZ][HI][LIM][FUEL][OIL][->]  (0-40px)      │
  ├─────────────────────────────────────────────────────────────┤
  │ [████████████████████░░░░░░░░░░░░] RPM BAR   (40-100px)    │
  ├─────────────────────────────────────────────────────────────┤
  │ ┌──────────┬────────────────┬──────────┐                   │
  │ │ COOLANT  │                │  8245    │                   │
  │ │ [████░░] │       4        │   RPM    │  (100-280px)      │
  │ │ OIL-P    │                │          │                   │
  │ │ [██████] │     GEAR       │   235    │                   │
  │ │ FUEL     │                │   KMH    │                   │
  │ │ [██░░░░] │                │          │                   │
  │ └──────────┴────────────────┴──────────┘                   │
  ├─────────────────────────────────────────────────────────────┤
  │ BRK [████████░░░░░░░░░] THR [░░░░░████████]  (280-320px)   │
  └─────────────────────────────────────────────────────────────┘

  SimHub JavaScript:
// Universal Telemetry Parser
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

  Date: 2026-01-04
*/

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "esp_task_wdt.h"

// ======================== HARDWARE PINS ========================
#define RST_PIN 8
#define DC_PIN 9
#define CS_PIN 10
#define MOSI_PIN 11
#define SCLK_PIN 12
#define LED_PIN 13

// ======================== COLOR PALETTE (VFD) ========================
#define COLOR_BG 0x0000       // Black
#define COLOR_VFD_MAIN 0x0679 // #04C4CA Teal (bright)
#define COLOR_VFD_GLOW 0x0235 // #01468A Darker teal for glow effect
#define COLOR_WARNING 0xFC60  // #FF8C00 Orange
#define COLOR_PEAK_RED 0xF802 // #FF0040 Peak Red (>95% RPM)
#define COLOR_YELLOW 0xFFE0   // #0xFFE0 Yellow (80-95% RPM)
#define COLOR_ORANGE 0xFC00   // #FF8000 Orange (88-92% RPM)
#define COLOR_GHOST 0x18E3    // #333333 Ghost (inactive)
#define COLOR_BLUE 0x2837     // #0x2837 Blue (high beam)
#define COLOR_GREEN 0x07E0    // Green for throttle

// ======================== LGFX CONFIGURATION ========================
class LGFX : public lgfx::LGFX_Device
{
public:
  lgfx::Panel_ST7796 _panel;
  lgfx::Bus_SPI _bus;

  LGFX()
  {
    // SPI Bus
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 80000000;
      cfg.freq_read = 0;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.pin_sclk = SCLK_PIN;
      cfg.pin_mosi = MOSI_PIN;
      cfg.pin_miso = -1;
      cfg.pin_dc = DC_PIN;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    // Panel ST7796
    {
      auto cfg = _panel.config();
      cfg.pin_cs = CS_PIN;
      cfg.pin_rst = RST_PIN;
      cfg.pin_busy = -1;
      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.readable = false;
      cfg.rgb_order = false;
      cfg.invert = false;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }

    setPanel(&_panel);
  }
};

LGFX lcd;
LGFX_Sprite sprite(&lcd);

// ======================== BACKLIGHT ========================
constexpr int BL_CH = 0;

void backlight_init()
{
  ledcSetup(BL_CH, 5000, 8);
  ledcAttachPin(LED_PIN, BL_CH);
  ledcWrite(BL_CH, 255); // 100% brightness
}

// ======================== TELEMETRY DATA ========================
struct TelemetryData
{
  float speed = 0.0f;
  float rpm = 0.0f;
  float maxRpm = 8000.0f;
  String gear = "N";       // String for ETS "R1", "R2"
  float fuelPct = -1.0f;   // -1 = no data
  float waterTemp = -1.0f; // -1 = no data
  float oilTemp = -1.0f;   // -1 = no data
  float oilPress = -1.0f;  // -1 = no data
  float throttle = 0.0f;
  float brake = 0.0f;

  bool leftTurn = false;
  bool rightTurn = false;
  bool abs = false;
  bool tc = false;            // TC separate from ABS
  bool parkingLights = false; // Running lights
  bool lowBeam = false;
  bool highBeam = false;
  bool hazard = false;
  bool handbrake = false;     // Parking brake
  bool cruiseControl = false; // Cruise/limiter
  bool lowFuel = false;
  bool oilWarning = false;
} telemetry;

// Light support tracking (from dashboard_galant)
bool parkingSupportedByGame = false;
bool lowBeamSupportedByGame = false;
bool highBeamSupportedByGame = false;

// Default values (from dashboard_galant)
const float DEFAULT_FUEL_PCT = 75.0f;
const float DEFAULT_WATER_TEMP = 90.0f;
const float DEFAULT_OIL_PRESS = 3.0f;

float smoothRpm = 0.0f;
float smoothSpeed = 0.0f;
float smoothThrottle = 0.0f;
float smoothBrake = 0.0f;

// ======================== CONSTANTS ========================
#define WDT_TIMEOUT 5
#define FRAME_INTERVAL 33
#define BLINK_INTERVAL 500
#define SIMHUB_TIMEOUT 3000
#define EMA_ALPHA 0.4f // was 0.3f

enum SystemState
{
  STATE_BOOT,
  STATE_RUNNING,
  STATE_ERROR
};
SystemState sysState = STATE_BOOT;
unsigned long lastFrameTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastSimhubPacket = 0;
bool blinkState = false;

// ======================== LAYOUT CONSTANTS ========================
#define ICON_SPACING 40 // 480px / 12 icons = 40px per icon
#define ICON_Y_CENTER 20

// ======================== HELPER FUNCTIONS ========================
inline float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline float constrainFloat(float x, float min_val, float max_val)
{
  if (x < min_val)
    return min_val;
  if (x > max_val)
    return max_val;
  return x;
}

inline float emaFilter(float prev, float curr, float alpha)
{
  return alpha * curr + (1.0f - alpha) * prev;
}

void drawVFDText(const String &text, int x, int y, uint16_t color)
{
  // 1. Extract raw R, G, B (0-255) from RGB565 format
  uint8_t r = (color >> 11) << 3;
  uint8_t g = ((color >> 5) & 0x3F) << 2;
  uint8_t b = (color & 0x1F) << 3;

  // 2. Calculate layer colors (proportional dimming)
  // Outer glow: ~20% brightness
  uint16_t glowOuter = sprite.color565(r * 0.2, g * 0.2, b * 0.2);
  // Inner glow: ~50% brightness
  uint16_t glowInner = sprite.color565(r * 0.5, g * 0.5, b * 0.5);

  // 3. DRAW LAYERS (from far to near)

  // STEP 1: Outer glow (2px blur)
  sprite.setTextColor(glowOuter);
  for (int dx = -2; dx <= 2; dx++)
  {
    for (int dy = -2; dy <= 2; dy++)
    {
      if (dx != 0 || dy != 0)
        sprite.drawString(text, x + dx, y + dy);
    }
  }

  // STEP 2: Inner glow (1px blur) - create density
  sprite.setTextColor(glowInner);
  sprite.drawString(text, x - 1, y);
  sprite.drawString(text, x + 1, y);
  sprite.drawString(text, x, y - 1);
  sprite.drawString(text, x, y + 1);

  // STEP 3: Core (main text)
  sprite.setTextColor(color);
  sprite.drawString(text, x, y);
}

void computeIndicators()
{
  // Software hazard (if SimHub didn't send HAZARD flag)
  if (telemetry.leftTurn && telemetry.rightTurn)
  {
    telemetry.hazard = true;
  }

  telemetry.lowFuel = (telemetry.fuelPct < 15.0f);
  telemetry.oilWarning = (telemetry.oilPress < 1.5f);

  // Light fallback logic (from dashboard_galant)
  // Priority 1: All supported - use as-is
  if (parkingSupportedByGame && lowBeamSupportedByGame && highBeamSupportedByGame)
  {
    return; // Perfect! Use real data
  }

  // Priority 2: Only LowBeam + HighBeam (no Parking) - enable Parking when LowBeam on
  if (!parkingSupportedByGame && lowBeamSupportedByGame && highBeamSupportedByGame)
  {
    telemetry.parkingLights = telemetry.lowBeam;
    return;
  }

  // Priority 3: Only LowBeam (no Parking, no High) - enable Parking
  if (!parkingSupportedByGame && lowBeamSupportedByGame && !highBeamSupportedByGame)
  {
    telemetry.parkingLights = telemetry.lowBeam;
    return;
  }

  // Priority 4: No light data at all - use engine running as indicator
  if (!parkingSupportedByGame && !lowBeamSupportedByGame && !highBeamSupportedByGame)
  {
    if (telemetry.rpm > 0)
    { // Engine running
      telemetry.parkingLights = true;
      telemetry.lowBeam = true;
    }
    else
    {
      telemetry.parkingLights = false;
      telemetry.lowBeam = false;
    }
    telemetry.highBeam = false; // Never assume high beam
    return;
  }
}

// Reset telemetry fields to sane defaults (used when losing connection / before new data)
void resetTelemetry()
{
  telemetry.speed = 0.0f;
  telemetry.rpm = 0.0f;
  telemetry.maxRpm = 8000.0f;
  telemetry.gear = "N";

  // Use -1 for values that indicate "no data available" (matches struct defaults)
  telemetry.fuelPct = -1.0f;
  telemetry.waterTemp = -1.0f;
  telemetry.oilTemp = -1.0f;
  telemetry.oilPress = -1.0f;

  telemetry.throttle = 0.0f;
  telemetry.brake = 0.0f;

  // Flags
  telemetry.leftTurn = false;
  telemetry.rightTurn = false;
  telemetry.abs = false;
  telemetry.tc = false;
  telemetry.parkingLights = false;
  telemetry.lowBeam = false;
  telemetry.highBeam = false;
  telemetry.hazard = false;
  telemetry.handbrake = false;
  telemetry.cruiseControl = false;
  telemetry.lowFuel = false;
  telemetry.oilWarning = false;
}

// ======================== DRAWING FUNCTIONS ========================

void drawIcon(int index, bool active, int iconType, uint16_t color)
{
  int x = ICON_SPACING / 2 + index * ICON_SPACING;
  int y = ICON_Y_CENTER;
  int size = 30; // Square size
  int x1 = x - size / 2;
  int y1 = y - size / 2;

  uint16_t drawColor = active ? color : COLOR_GHOST;

  // 1. DRAW SQUARE CONTAINER
  // Draw the border. If active - make it brighter.
  sprite.drawRect(x1, y1, size, size, drawColor);

  // Optional: slight fill of the active square background for VFD effect (very dim)
  if (active)
  {
    // sprite.fillRect(x1 + 1, y1 + 1, size - 2, size - 2, COLOR_VFD_GLOW);
  }

  // 2. DRAW SYMBOLS INSIDE
  switch (iconType)
  {
  case 0: // Left turn signal
    sprite.fillTriangle(x - 10, y, x - 2, y - 7, x - 2, y + 7, drawColor);
    sprite.fillRect(x - 2, y - 3, 10, 6, drawColor);
    break;

  case 1: // ABS
  case 2: // TC
    sprite.setTextSize(1);
    sprite.setTextColor(drawColor);
    sprite.setTextDatum(middle_center);
    sprite.drawString(iconType == 1 ? "ABS" : "TC", x, y);
    sprite.drawArc(x, y, 11, 13, 135, 225, drawColor); // Left arc
    sprite.drawArc(x, y, 11, 13, 315, 45, drawColor);  // Right arc
    break;

  case 3: // Parking lights
    sprite.drawCircle(x - 5, y, 4, drawColor);
    sprite.drawCircle(x + 5, y, 4, drawColor);
    for (int i = -1; i <= 1; i++)
    {
      sprite.drawLine(x - 9, y + (i * 4), x - 13, y + (i * 6), drawColor);
      sprite.drawLine(x + 9, y + (i * 4), x + 13, y + (i * 6), drawColor);
    }
    break;

  case 4: // Low beam
    sprite.drawRoundRect(x - 3, y - 7, 10, 14, 3, drawColor);
    sprite.fillRect(x - 3, y - 7, 5, 14, drawColor);
    for (int i = 0; i < 3; i++)
      sprite.drawLine(x - 6, y - 5 + (i * 5), x - 12, y - 2 + (i * 5), drawColor);
    break;

  case 5: // Hazard lights
    sprite.drawTriangle(x, y - 10, x - 10, y + 8, x + 10, y + 8, drawColor);
    sprite.drawTriangle(x, y - 6, x - 6, y + 5, x + 6, y + 5, drawColor);
    break;

  case 6: // High beam
    sprite.drawRoundRect(x - 3, y - 7, 10, 14, 3, drawColor);
    sprite.fillRect(x - 3, y - 7, 5, 14, drawColor);
    for (int i = 0; i < 3; i++)
      sprite.drawLine(x - 6, y - 5 + (i * 5), x - 13, y - 5 + (i * 5), drawColor);
    break;

  case 7: // Cruise / Pit limiter
    sprite.drawArc(x, y + 2, 9, 11, 220, 140, drawColor);
    sprite.drawLine(x, y + 2, x + 6, y - 4, drawColor);
    sprite.fillCircle(x, y + 2, 2, drawColor);
    break;

  case 8: // Handbrake (P)
    sprite.drawCircle(x, y, 9, drawColor);
    sprite.setTextSize(1);
    sprite.setTextColor(drawColor);
    sprite.setTextDatum(middle_center);
    sprite.drawString("P", x, y);
    sprite.drawArc(x, y, 11, 13, 135, 225, drawColor);
    sprite.drawArc(x, y, 11, 13, 315, 45, drawColor);
    break;

  case 9: // Low fuel level
    sprite.drawRect(x - 6, y - 7, 9, 15, drawColor);
    sprite.fillRect(x - 4, y - 5, 5, 4, drawColor == COLOR_GHOST ? COLOR_BG : drawColor);
    sprite.drawLine(x + 3, y - 4, x + 7, y - 4, drawColor);
    sprite.drawLine(x + 7, y - 4, x + 7, y + 4, drawColor);
    sprite.fillRect(x + 5, y + 4, 4, 2, drawColor);
    break;
  case 10: // Oil pressure
    sprite.drawEllipse(x - 1, y + 3, 9, 5, drawColor);
    sprite.drawArc(x - 8, y + 1, 5, 7, 180, 300, drawColor);
    sprite.drawLine(x + 7, y + 1, x + 12, y - 3, drawColor);
    sprite.fillCircle(x + 12, y, 2, drawColor);
    break;
  case 11: // Right turn signal
    sprite.fillTriangle(x + 10, y, x + 2, y - 7, x + 2, y + 7, drawColor);
    sprite.fillRect(x - 8, y - 3, 10, 6, drawColor);
    break;
  }
}

// Draw icons row (12 icons with graphical symbols)
void drawIconsRow()
{
  drawIcon(0, telemetry.leftTurn, 0, COLOR_GREEN);         // Left arrow (green) - SimHub sends blink
  drawIcon(1, telemetry.abs, 1, COLOR_WARNING);            // ABS
  drawIcon(2, telemetry.tc, 2, COLOR_WARNING);             // TC
  drawIcon(3, telemetry.parkingLights, 3, COLOR_GREEN);    // Parking lights (green)
  drawIcon(4, telemetry.lowBeam, 4, COLOR_GREEN);          // Low beam (green)
  drawIcon(5, telemetry.hazard, 5, COLOR_WARNING);         // Hazard - SimHub sends blink
  drawIcon(6, telemetry.highBeam, 6, COLOR_BLUE);          // High beam
  drawIcon(7, telemetry.cruiseControl, 7, COLOR_VFD_MAIN); // Cruise
  drawIcon(8, telemetry.handbrake, 8, COLOR_PEAK_RED);     // Handbrake
  drawIcon(9, telemetry.lowFuel, 9, COLOR_WARNING);        // Fuel - our blink (computed warning)
  drawIcon(10, telemetry.oilWarning, 10, COLOR_PEAK_RED);  // Oil - our blink (computed warning)
  drawIcon(11, telemetry.rightTurn, 11, COLOR_GREEN);      // Right arrow (green) - SimHub sends blink
}

// Draw RPM bar (40-100px, full width) - 150 segments
void drawRPMBar()
{
  float rpmPct = (telemetry.maxRpm > 0) ? (smoothRpm / telemetry.maxRpm * 100.0f) : 0;
  rpmPct = constrainFloat(rpmPct, 0, 100);

  int activeSegments = (int)((rpmPct / 100.0f) * 150.0f); // 0-150 segments

  // First draw all 150 outlines (divisions)
  for (int i = 0; i < 150; i++)
  {
    int x = i * 3.2f;
    sprite.drawRect(x, 40, 3, 60, COLOR_GHOST);
  }

  // Then fill active segments with 1px padding so the outline remains visible
  for (int i = 0; i < activeSegments; i++)
  {
    int x = i * 3.2f;
    float segmentPct = ((float)i / 150.0f) * 100.0f;
    uint16_t color;
    if (segmentPct >= 92.0f)
    {
      color = (segmentPct >= 96.0f && blinkState) ? COLOR_PEAK_RED : COLOR_PEAK_RED;
    }
    else if (segmentPct >= 88.0f)
    {
      color = COLOR_ORANGE;
    }
    else if (segmentPct >= 80.0f)
    {
      color = COLOR_YELLOW;
    }
    else
    {
      color = COLOR_GREEN;
    }
    // Fill active segments with 1px padding so the outline remains visible
    sprite.fillRect(x + 1, 41, 1, 58, color);
  }
}

void drawMiniBarH(int x, int y, int w, int h, float pct, float val, const char *label, const char *unit, uint16_t color)
{
  sprite.setTextSize(1);
  sprite.setTextColor(COLOR_VFD_MAIN);
  sprite.setTextDatum(top_left);
  sprite.setFont(&fonts::FreeSans9pt7b);
  // sprite.setFont(&fonts::FreeMono9pt7b); // Use monospace font so digits don't "jump"
  // sprite.setFont(&fonts::DejaVu9); // Use monospace font so digits don't "jump"

  // 1. Draw instrument label
  sprite.drawString(label, x, y);

  int barY = y + 18;
  int activeSegments = (int)(pct / 10.0f); // 10 segments

  // 2. Draw bar segments (make them slightly shorter in width)
  for (int i = 0; i < 10; i++)
  {
    int segX = x + i * 9; // Step 9px instead of 10px
    uint16_t segColor = color;

    // 1. For COOLANT and OIL TEMP:
    // Only the last 2 segments (i==8 or i==9) are red when filled
    // Only the first 2 filled segments (i==0 or i==1) are blue when val < 70
    if (strcmp(label, "COOLANT") == 0 || strcmp(label, "OIL TEMP") == 0)
    {
      if (i >= 8)
      {
        segColor = COLOR_PEAK_RED;
      }
      else if (val < 70 && i < activeSegments && (i == 0 || i == 1))
      {
        segColor = 0x5ADF; // Blue for the first two filled segments when cold
      }
      else
      {
        segColor = COLOR_VFD_MAIN;
      }
    }

    // 2. For OIL PRESS:
    // If only 1 segment is filled — it is red; if more — all normal, except the last (i==9), which is red when filled
    if (strcmp(label, "OIL PRESS") == 0)
    {
      if (activeSegments == 1)
      {
        segColor = (i == 0) ? COLOR_PEAK_RED : COLOR_VFD_MAIN;
      }
      else if (activeSegments > 1)
      {
        segColor = (i == 9) ? COLOR_PEAK_RED : COLOR_VFD_MAIN;
      }
    }

    // 3. For FUEL:
    // If only 1 segment — it is red
    // If 2 — both are orange
    // If more — all are normal
    if (strcmp(label, "FUEL") == 0)
    {
      if (activeSegments == 1)
      {
        segColor = (i == 0) ? COLOR_PEAK_RED : COLOR_VFD_MAIN;
      }
      else if (activeSegments == 2)
      {
        segColor = (i == 0 || i == 1) ? COLOR_ORANGE : COLOR_VFD_MAIN;
      }
      else
      {
        segColor = COLOR_VFD_MAIN;
      }
    }

    if (i < activeSegments)
    {
      sprite.fillRect(segX, barY, 7, h, segColor);
    }
    else
    {
      sprite.drawRect(segX, barY, 7, h, COLOR_GHOST);
    }
  }

  // 3. Draw the value and unit to the right of the bar
  sprite.setTextDatum(middle_left);
  String valueStr;

  // Formatting: for oil pressure use 1 decimal place, otherwise integers
  if (val < 0)
    valueStr = "---";
  else if (pct == 0 && val == 0)
    valueStr = "0";
  else if (strcmp(label, "OIL PRESS") == 0)
    valueStr = String(val, 1);
  else
    valueStr = String((int)val);

  // Draw the value (bright)
  sprite.setTextColor(color);
  sprite.drawString(valueStr, x + 95, barY + h / 2);

  // Draw the unit (dimmer, using COLOR_GHOST)
  sprite.setTextColor(COLOR_GHOST);
  // sprite.setFont(nullptr); // Return to default font for small text
  sprite.setTextSize(0.99);
  sprite.setFont(&fonts::FreeSansOblique9pt7b);
  sprite.drawString(unit, x + 95 + (valueStr.length() * 10), barY + h / 2);
}

void drawGauges()
{
  int x = 10;
  int y = 105;
  int w = 85; // Strip width slightly smaller
  int h = 10; // Height slightly larger for readability
  int stepY = 45;

  // COOLANT
  float waterTemp = (telemetry.waterTemp < 0) ? 90.0f : telemetry.waterTemp;
  float coolPct = constrainFloat(mapFloat(waterTemp, 40, 120, 0, 100), 0, 100);
  uint16_t coolColor = (waterTemp > 105) ? COLOR_PEAK_RED : COLOR_VFD_MAIN;
  drawMiniBarH(x, y, w, h, coolPct, waterTemp, "COOLANT", "C", coolColor);

  // OIL TEMP
  float oilTemp = (telemetry.oilTemp < 0) ? 95.0f : telemetry.oilTemp;
  float oilTPct = constrainFloat(mapFloat(oilTemp, 50, 150, 0, 100), 0, 100);
  uint16_t oilTColor = (oilTemp < 70) ? 0x5ADF : (oilTemp > 130 ? COLOR_PEAK_RED : COLOR_VFD_MAIN);
  drawMiniBarH(x, y + stepY, w, h, oilTPct, oilTemp, "OIL TEMP", "C", oilTColor);

  // OIL PRESSURE
  float oilPress = (telemetry.oilPress < 0) ? 0.0f : telemetry.oilPress;
  float oilPPct = constrainFloat(mapFloat(oilPress, 0, 11, 0, 100), 0, 100);
  uint16_t oilPColor = (oilPress < 1.5f) ? COLOR_PEAK_RED : COLOR_VFD_MAIN;
  drawMiniBarH(x, y + stepY * 2, w, h, oilPPct, oilPress, "OIL PRESS", "BAR", oilPColor);

  // FUEL
  float fuelPct = (telemetry.fuelPct < 0) ? 75.0f : telemetry.fuelPct;
  uint16_t fuelColor = (fuelPct < 15) ? COLOR_WARNING : COLOR_VFD_MAIN;
  drawMiniBarH(x, y + stepY * 3, w, h, fuelPct, fuelPct, "FUEL", "%", fuelColor);
}

// Draw GEAR (center, large) - String support for ETS2 R1, R2, etc.
void drawGear()
{
  int x = 180;
  int y = 105;           // 230
  sprite.setTextSize(1); // Large size
  sprite.setTextColor(COLOR_VFD_MAIN);
  sprite.setTextDatum(top_left);
  sprite.setFont(&fonts::FreeSans9pt7b);
  sprite.drawString("GEAR", x, y);

  sprite.setTextDatum(middle_center);
  sprite.setFont(&fonts::Orbitron_Light_32); // Digital 7-segment style
  sprite.setTextColor(COLOR_YELLOW);
  sprite.setTextSize(3); // Large size
  x = 210;
  y = 160;

  String gearStr = telemetry.gear;
  // If string is empty — show N
  if (gearStr.length() == 0)
  {
    gearStr = "N";
  }
  // If gear is negative (e.g., "-1", "-2"), show only "R"
  else if (gearStr.toInt() < 0)
  {
    gearStr = "R";
  }

  // Draw with glow effect
  drawVFDText(gearStr, x, y, COLOR_YELLOW);

  sprite.setFont(nullptr); // Reset to default
}

int getDigitWidth(const lgfx::GFXfont *font)
{
  sprite.setFont(font);
  return sprite.textWidth("8");
}

String formatFixedDigits(int value, int digits)
{
  String s = String(value);
  while (s.length() < digits)
    s = " " + s; // space = empty digit
  return s;
}

void drawSegmentNumber(
    int value,
    int digits,
    int x,
    int y,
    int digitWidth,
    const lgfx::GFXfont *font,
    uint16_t mainColor,
    uint16_t ghostColor,
    char ghostChar = '8')
{
  String text = formatFixedDigits(value, digits);

  sprite.setFont(font);
  sprite.setTextDatum(top_left);

  for (int i = 0; i < digits; i++)
  {
    int cellX = x + i * digitWidth;
    char c = text[i];

    // Ghost (across the whole cell)
    sprite.setTextColor(ghostColor);
    sprite.drawString(String(ghostChar), cellX, y);
    // Main digit — RIGHT-aligned
    if (c != ' ')
    {
      String s(c);
      int charW = sprite.textWidth(s);

      int alignedX = cellX + digitWidth - charW; // right align
      drawVFDText(s, alignedX, y, mainColor);
    }
  }
}

// Draw right info (RPM numeric, Speed numeric)
void drawInfo()
{
  int x = 270;

  // RPM numeric
  sprite.setColor(COLOR_VFD_MAIN);
  sprite.setFont(&fonts::Orbitron_Light_24);
  sprite.setTextSize(2);
  sprite.setTextDatum(top_left);
  int rpmInt = (int)smoothRpm;
  // drawVFDText(String(rpmInt), x, 110, COLOR_VFD_MAIN); //x, 100
  int digitW = getDigitWidth(&fonts::Orbitron_Light_24);
  drawSegmentNumber(
      rpmInt,
      5, // 00000–99999
      x,
      110,
      digitW,
      &fonts::Orbitron_Light_24,
      COLOR_VFD_MAIN,
      COLOR_GHOST,
      '8');

  // Label
  sprite.setColor(COLOR_VFD_MAIN);
  sprite.setFont(&fonts::FreeSans9pt7b);
  sprite.setTextSize(1);
  sprite.setTextColor(COLOR_VFD_MAIN);
  sprite.drawString("RPM", x + 155, 105); // x + 130, 150

  // Speed KMH
  sprite.setColor(COLOR_VFD_MAIN);
  sprite.setFont(&fonts::Orbitron_Light_24);
  sprite.setTextSize(3.45);
  int speedInt = (int)smoothSpeed;
  digitW = getDigitWidth(&fonts::Orbitron_Light_24); // +4px spacing
  drawSegmentNumber(
      speedInt,
      3,
      x,
      170,
      digitW,
      &fonts::Orbitron_Light_24,
      COLOR_VFD_MAIN,
      COLOR_GHOST,
      '8');
  // drawVFDText(String(speedInt), x, 170, COLOR_VFD_MAIN); // x, 160

  // Label
  sprite.setColor(COLOR_VFD_MAIN);
  sprite.setFont(&fonts::FreeSans9pt7b);
  sprite.setTextSize(1.2);
  sprite.setTextColor(COLOR_VFD_MAIN);
  sprite.drawString("KMH", x + 150, 170); // x + 130, 230

  sprite.setFont(nullptr); // Reset
}

// Draw pedals (bottom) - 100 segments each
void drawPedals()
{
  int y = 285;
  int h = 30;

  // Brake (left half) - fills RIGHT to LEFT - 100 segments
  int brakeSegments = (int)smoothBrake; // 0-100
  sprite.setTextSize(1);
  sprite.setTextColor(COLOR_VFD_MAIN);
  sprite.drawString("BRK", 215, y - 10);

  // First draw all divisions (ghost lines)
  for (int i = 0; i < 100; i++)
  {
    int segX = 5 + (99 - i) * 2.3f;
    sprite.drawLine(segX, y, segX, y + h - 1, COLOR_GHOST);
  }
  // Then fill active segments with padding (keep divisions visible)
  for (int i = 0; i < brakeSegments; i++)
  {
    int segX = 5 + (99 - i) * 2.3f;
    sprite.fillRect(segX + 1, y + 1, 0, h - 2, COLOR_PEAK_RED); // 0px width so as not to overwrite the lines
    sprite.fillRect(segX, y + 1, 1, h - 2, COLOR_PEAK_RED);     // 1px width centered
  }

  // Throttle (right half) - fills LEFT to RIGHT - 100 segments
  int thrSegments = (int)smoothThrottle; // 0-100
  sprite.drawString("THR", 455, y - 10);

  for (int i = 0; i < 100; i++)
  {
    int segX = 245 + i * 2.3f;
    sprite.drawLine(segX, y, segX, y + h - 1, COLOR_GHOST);
  }
  for (int i = 0; i < thrSegments; i++)
  {
    int segX = 245 + i * 2.3f;
    sprite.fillRect(segX + 1, y + 1, 0, h - 2, COLOR_GREEN);
    sprite.fillRect(segX, y + 1, 1, h - 2, COLOR_GREEN);
  }
}

// Main dashboard update
void updateDashboard()
{
  sprite.fillSprite(COLOR_BG);

  drawIconsRow(); // 0-40px
  drawRPMBar();   // 40-100px
  drawGauges();   // Left side
  drawGear();     // Center
  drawInfo();     // Right side
  drawPedals();   // 280-320px
  sprite.pushSprite(0, 0);
}

// ======================== SIMHUB SERIAL PARSER ========================
void parseSimhubData(String data)
{
  int idx = 0;
  while (idx < data.length())
  {
    int sepIdx = data.indexOf(';', idx);
    if (sepIdx == -1)
      sepIdx = data.length();

    String pair = data.substring(idx, sepIdx);
    int colonIdx = pair.indexOf(':');
    if (colonIdx > 0)
    {
      String key = pair.substring(0, colonIdx);
      String value = pair.substring(colonIdx + 1);

      if (key == "KMH")
        telemetry.speed = value.toFloat();
      else if (key == "RPM")
        telemetry.rpm = value.toFloat();
      else if (key == "MAXRPM")
        telemetry.maxRpm = value.toFloat();
      else if (key == "GEAR")
        telemetry.gear = value;
      else if (key == "FUEL")
        telemetry.fuelPct = value.toFloat();
      else if (key == "WATER")
        telemetry.waterTemp = value.toFloat();
      else if (key == "OILTEMP")
        telemetry.oilTemp = value.toFloat();
      else if (key == "OILPRESS")
        telemetry.oilPress = value.toFloat();
      else if (key == "THROTTLE")
        telemetry.throttle = value.toFloat();
      else if (key == "BRAKE")
        telemetry.brake = value.toFloat();
      else if (key == "LEFTTURN")
        telemetry.leftTurn = (value == "1");
      else if (key == "RIGHTTURN")
        telemetry.rightTurn = (value == "1");
      else if (key == "ABS")
        telemetry.abs = (value == "1");
      else if (key == "TC")
        telemetry.tc = (value == "1");
      else if (key == "PARKLIGHT")
      {
        telemetry.parkingLights = (value == "1");
        if (value == "1")
          parkingSupportedByGame = true;
      }
      else if (key == "LOWBEAM")
      {
        telemetry.lowBeam = (value == "1");
        if (value == "1")
          lowBeamSupportedByGame = true;
      }
      else if (key == "HIGHBEAM")
      {
        telemetry.highBeam = (value == "1");
        if (value == "1")
          highBeamSupportedByGame = true;
      }
      else if (key == "HAZARD")
        telemetry.hazard = (value == "1");
      else if (key == "HANDBRAKE")
        telemetry.handbrake = (value == "1");
      else if (key == "CRUISE")
        telemetry.cruiseControl = (value == "1");
    }

    idx = sepIdx + 1;
  }

  lastSimhubPacket = millis();
}

void processSerial()
{
  static String inputBuffer = "";
  while (Serial.available())
  {
    char c = Serial.read();
    if (c == '\n')
    {
      inputBuffer.trim();
      if (inputBuffer.length() > 0)
      {
        resetTelemetry();
        parseSimhubData(inputBuffer);
      }
      inputBuffer = "";
    }
    else
    {
      inputBuffer += c;
    }
  }
}

// ======================== SETUP ========================
void setup()
{
  Serial.begin(115200);
  delay(1000); // CRITICAL: Wait for USB CDC to stabilize before SimHub connection
  Serial.println("\n=== VFD Digital Dashboard ===");

  // Watchdog
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("INFO: Watchdog enabled (5s)");

  // Display init
  lcd.init();
  lcd.setRotation(1); // Landscape
  lcd.fillScreen(COLOR_BG);

  // Backlight
  backlight_init();
  Serial.println("INFO: Backlight ON");

  // Sprite
  sprite.setPsram(false);
  if (!sprite.createSprite(480, 320))
  {
    Serial.println("ERROR: Sprite creation failed!");
    while (1)
      delay(1000);
  }
  Serial.println("INFO: Sprite created (PSRAM)");

  // ======================== WELCOME ANIMATION ========================
  // Phase 1: Title screen (1.5s) with VFD glow effect
  sprite.fillSprite(COLOR_BG);
  sprite.setFont(&fonts::Orbitron_Light_24); // Digital font
  sprite.setTextSize(1);
  sprite.setTextDatum(middle_center);
  drawVFDText("VFD DASHBOARD", 240, 140, COLOR_VFD_MAIN);
  sprite.setTextSize(1);
  drawVFDText("DIGITAL 2000", 240, 180, COLOR_VFD_MAIN);
  sprite.setFont(nullptr); // Reset
  sprite.setTextSize(1);
  sprite.setTextColor(COLOR_GHOST);
  sprite.drawString("Initializing systems...", 240, 220);
  sprite.pushSprite(0, 0);
  delay(1000);

  // Phase 2: PARALLEL ANIMATION (Dash Sweep)
  String gears[] = {"R", "N", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
  int gearIndex = 0;
  unsigned long gearChangeTime = millis();

  for (int frame = 0; frame <= 100; frame += 2)
  {
    sprite.fillSprite(COLOR_BG);
    float pct = frame;

    if (millis() - gearChangeTime > 300 && gearIndex < 12)
    {
      gearIndex++;
      gearChangeTime = millis();
    }

    // 1. Animate needles and values
    telemetry.rpm = pct * 120.0f;
    telemetry.maxRpm = 12000.0f;
    telemetry.speed = pct * 3.0f;
    telemetry.gear = gears[gearIndex];
    telemetry.fuelPct = mapFloat(pct, 0, 100, 0, 100);
    telemetry.waterTemp = mapFloat(pct, 0, 100, 40, 120);
    telemetry.oilTemp = mapFloat(pct, 0, 100, 50, 150);
    telemetry.oilPress = mapFloat(pct, 0, 100, 0, 11);
    telemetry.throttle = pct;
    telemetry.brake = pct;

    // 2. INDICATORS LOGIC
    bool flash = (frame % 20 < 10); // Flash state (500ms on/off)
    blinkState = flash;             // Store in global variable for rendering

    telemetry.leftTurn = true;  // was a flash
    telemetry.rightTurn = true; // was a flash
    telemetry.hazard = true;    // was a flash
    telemetry.abs = true;
    telemetry.tc = true;
    telemetry.parkingLights = true;
    telemetry.lowBeam = true;
    telemetry.highBeam = true;
    telemetry.handbrake = true;
    telemetry.cruiseControl = true;
    telemetry.lowFuel = true;
    telemetry.oilWarning = true;

    // Update smoothed values for rendering
    smoothRpm = telemetry.rpm;
    smoothSpeed = telemetry.speed;
    smoothThrottle = telemetry.throttle;
    smoothBrake = telemetry.brake;

    // 3. RENDERING
    drawIconsRow();
    drawRPMBar();
    drawGauges();
    drawGear();
    drawInfo();
    drawPedals();

    sprite.pushSprite(0, 0);
    delay(20);
    esp_task_wdt_reset();
  }

  // === END OF ANIMATION: TURN EVERYTHING OFF ===
  delay(300); // Short pause at peak while everything is lit
  // Reset telemetry state to defaults before waiting for SimHub
  resetTelemetry();

  // ======================== END WELCOME ANIMATION ========================

  sysState = STATE_RUNNING;
  lastSimhubPacket = millis();

  Serial.println("INFO: Dashboard ready - waiting for SimHub");
}

// ======================== LOOP ========================
void loop()
{
  esp_task_wdt_reset();
  processSerial();

  unsigned long now = millis();

  // Blink state
  if (now - lastBlinkTime >= BLINK_INTERVAL)
  {
    lastBlinkTime = now;
    blinkState = !blinkState;
  }

  // SimHub connection check
  bool simhubConnected = (now - lastSimhubPacket) < SIMHUB_TIMEOUT;

  if (sysState == STATE_RUNNING && simhubConnected)
  {
    // EMA filter
    smoothRpm = emaFilter(smoothRpm, telemetry.rpm, EMA_ALPHA);
    smoothSpeed = emaFilter(smoothSpeed, telemetry.speed, EMA_ALPHA);
    smoothThrottle = emaFilter(smoothThrottle, telemetry.throttle, EMA_ALPHA * 1.5f);
    smoothBrake = emaFilter(smoothBrake, telemetry.brake, EMA_ALPHA * 1.5f);

    computeIndicators();
  }
  else
  {
    // Decay to zero on disconnect
    smoothRpm = emaFilter(smoothRpm, 0, EMA_ALPHA * 0.5f);
    smoothSpeed = emaFilter(smoothSpeed, 0, EMA_ALPHA * 0.5f);
    smoothThrottle = emaFilter(smoothThrottle, 0, EMA_ALPHA);
    smoothBrake = emaFilter(smoothBrake, 0, EMA_ALPHA);
  }

  // Frame rate control (30 FPS)
  if (now - lastFrameTime >= FRAME_INTERVAL)
  {
    lastFrameTime = now;
    updateDashboard();
  }

  delay(1);
}
