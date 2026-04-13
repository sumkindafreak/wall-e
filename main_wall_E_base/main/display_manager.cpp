// ============================================================
//  WALL-E Display Manager Implementation
//  ST7789 240x240 — same init pattern as working display_control.cpp
//  Uses Adafruit_ST7789 + Adafruit_GFX (no TFT_eSPI)
// ============================================================

#include "display_manager.h"
#include "wifi_manager.h"
#include "battery_monitor.h"
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ============================================================
//  Colour Palette — matches WebUI dark theme
// ============================================================
#define C_BG        0x0861   // #0d0f14
#define C_SURFACE   0x0CA3   // #13151e
#define C_SURFACE2  0x0D04   // #1a1d28
#define C_BORDER    0x1284   // #252836
#define C_ACCENT    0xFB04   // #f5a623 (orange)
#define C_TXT       0xDEF7   // #dde1f0
#define C_TXT_DIM   0x2B10   // #555e80
#define C_OK        0x1F8A   // #3ddc84 (green)
#define C_STOP      0xE189   // #e63946 (red)
#define C_WARN      0xFB04   // same as accent
#define C_BLACK     0x0000
#define C_WHITE     0xFFFF

// ============================================================
//  Layout constants (240x240)
// ============================================================
#define W   240
#define H   240
#define HDR_Y       0
#define HDR_H       36
#define CMD_Y       48
#define CMD_H       68
#define SPD_Y       128
#define SPD_H       30
#define BAT_Y       170
#define BAT_H       28
#define WIFI_Y      210
#define WIFI_H      28

// ============================================================
//  State
// ============================================================
static Adafruit_ST7789* tft = nullptr;
static GFXcanvas16* sprCmd  = nullptr;
static GFXcanvas16* sprSpd  = nullptr;
static GFXcanvas16* sprBat  = nullptr;
static GFXcanvas16* sprWifi = nullptr;

static const int SPR_W = W - 24;

static DriveCommand _cmd   = CMD_IDLE;
static uint8_t      _speed = 200;
static float        _joyX  = 0, _joyY = 0;  // tank stick position -1..1 for smooth display
static bool         _cmdDirty = true, _spdDirty = true, _batDirty = true, _wifiDirty = true;
static unsigned long _lastDraw = 0;
#define DRAW_INTERVAL_MS  80

/* ---- Chest TFT power-on sequence (every cold boot) ---- */
#define BOOT_CHARGE_PHASE_MS   3200u  /* bar fills 0 -> measured % */
#define BOOT_HOLD_PHASE_MS     4200u  /* hold so operator can read V/A/% */
#define BOOT_WAKE_PHASE_MS     1500u  /* short “wake up” animation */
#define BOOT_RESAMPLE_MS       450u   /* refresh ADC during charge phase */
#define BOOT_FRAME_MS          26u    /* ~38 fps during boot loops */

static char         _toastBuf[48] = "";
static unsigned long _toastUntil = 0;

// ============================================================
//  Helpers — text with datum (Adafruit_GFX has setCursor + print)
// ============================================================
enum { DATUM_TL, DATUM_TR, DATUM_BC, DATUM_MC };

static void bootDrawStringMC(int16_t y, const char* s, uint8_t textSize, uint16_t fg, uint16_t bg) {
  tft->setTextSize(textSize);
  tft->setTextColor(fg, bg);
  int16_t x1, y1;
  uint16_t tw, th;
  tft->getTextBounds(s, 0, 0, &x1, &y1, &tw, &th);
  tft->setCursor((W - (int)tw) / 2, y);
  tft->print(s);
}

/** Charging-style screen: big battery, fill only up to real SOC (fillPct 0..100 for animation). */
static void drawBootChargeScreen(int fillPct, const BatteryData& bat) {
  tft->fillScreen(C_BG);
  tft->setTextSize(1);
  tft->setTextColor(C_TXT_DIM, C_BG);
  bootDrawStringMC(16, "SOLAR CHARGE / BOOT", 1, C_TXT_DIM, C_BG);
  tft->setTextColor(C_ACCENT, C_BG);
  bootDrawStringMC(44, "WALL-E", 2, C_ACCENT, C_BG);

  int bx = 40, by = 92, bw = W - 80, bh = 52;
  tft->drawRoundRect(bx, by, bw, bh, 8, C_BORDER);
  tft->fillRoundRect(bx + 3, by + 3, bw - 6, bh - 6, 6, C_SURFACE2);
  int capW = 10;
  tft->fillRoundRect(bx + bw, by + 14, capW, bh - 28, 3, C_BORDER);

  int innerW = bw - 12;
  int innerH = bh - 12;
  int fw = (innerW * fillPct) / 100;
  if (fw < 0) fw = 0;
  if (fw > innerW) fw = innerW;
  uint16_t fcol = C_OK;
  if (bat.valid) {
    if (bat.status == BAT_CRITICAL) fcol = C_STOP;
    else if (bat.status == BAT_WARNING) fcol = C_WARN;
  } else {
    fcol = C_TXT_DIM;
  }
  if (fw > 2)
    tft->fillRoundRect(bx + 6, by + 6, fw, innerH, 4, fcol);

  char line[48];
  if (bat.valid) {
    snprintf(line, sizeof(line), "%.2f V   %.2f A   %d%%", bat.voltage, bat.currentA, bat.percent);
  } else {
    snprintf(line, sizeof(line), "%.2f V   %.2f A   --%%", bat.voltage, bat.currentA);
  }
  tft->setTextSize(1);
  tft->setTextColor(C_TXT, C_BG);
  bootDrawStringMC(158, line, 1, C_TXT, C_BG);

  tft->setTextColor(C_TXT_DIM, C_BG);
  bootDrawStringMC(182, "Reading power bus...", 1, C_TXT_DIM, C_BG);
}

static void drawBootHoldScreen(const BatteryData& bat) {
  tft->fillScreen(C_BG);
  tft->setTextColor(C_ACCENT, C_BG);
  bootDrawStringMC(28, "CHARGE LEVEL", 2, C_ACCENT, C_BG);
  char line[48];
  if (bat.valid) {
    snprintf(line, sizeof(line), "%.2f V  |  %.2f A", bat.voltage, bat.currentA);
  } else {
    snprintf(line, sizeof(line), "%.2f V  |  %.2f A (sense?)", bat.voltage, bat.currentA);
  }
  tft->setTextSize(1);
  tft->setTextColor(C_TXT, C_BG);
  bootDrawStringMC(64, line, 1, C_TXT, C_BG);

  int bx = 30, by = 92, bw = W - 60, bh = 36;
  tft->drawRoundRect(bx, by, bw, bh, 6, C_BORDER);
  tft->fillRoundRect(bx + 3, by + 3, bw - 6, bh - 6, 4, C_SURFACE2);
  if (bat.valid) {
    int fw = ((bw - 12) * bat.percent) / 100;
    uint16_t col = (bat.status == BAT_CRITICAL) ? C_STOP : (bat.status == BAT_WARNING) ? C_WARN : C_OK;
    if (fw > 2) tft->fillRoundRect(bx + 6, by + 6, fw, bh - 12, 3, col);
    char pb[16];
    snprintf(pb, sizeof(pb), "%d %%", bat.percent);
    tft->setTextSize(2);
    tft->setTextColor(C_TXT, C_SURFACE2);
    bootDrawStringMC(98, pb, 2, C_TXT, C_SURFACE2);
  } else {
    tft->setTextSize(2);
    bootDrawStringMC(98, "-- %", 2, C_TXT_DIM, C_SURFACE2);
  }

  tft->setTextSize(1);
  tft->setTextColor(C_TXT_DIM, C_BG);
  bootDrawStringMC(200, "Stand by — coming online", 1, C_TXT_DIM, C_BG);
}

/** Quick “wake up”: eyes pop + status line, then hand off to layered UI. */
static void drawBootWakeScreen(uint32_t elapsed) {
  tft->fillScreen(C_BG);
  int cx = W / 2;
  int eyeY = 100;
  int spread = (int)(elapsed * 28 / BOOT_WAKE_PHASE_MS);
  if (spread > 28) spread = 28;
  int ex = 72 - spread;
  tft->fillCircle(cx - ex, eyeY, 18 + (spread / 4), C_SURFACE2);
  tft->fillCircle(cx + ex, eyeY, 18 + (spread / 4), C_SURFACE2);
  tft->drawCircle(cx - ex, eyeY, 18 + (spread / 4), C_BORDER);
  tft->drawCircle(cx + ex, eyeY, 18 + (spread / 4), C_BORDER);
  tft->fillCircle(cx - ex, eyeY, 8, C_ACCENT);
  tft->fillCircle(cx + ex, eyeY, 8, C_ACCENT);

  tft->setTextSize(2);
  tft->setTextColor(C_OK, C_BG);
  bootDrawStringMC(168, "ONLINE", 2, C_OK, C_BG);
  tft->setTextSize(1);
  tft->setTextColor(C_TXT_DIM, C_BG);
  bootDrawStringMC(198, "Systems waking...", 1, C_TXT_DIM, C_BG);
}

static void displayRunPowerOnSequence(void) {
  Serial.println(F("[Display] Boot sequence: charge animation + telemetry hold + wake"));

  batterySetVerboseSampleLog(false);
  batterySampleNow();
  unsigned long lastResample = millis();

  unsigned long tCharge = millis();
  while (millis() - tCharge < BOOT_CHARGE_PHASE_MS) {
    unsigned long el = millis() - tCharge;
    if (millis() - lastResample >= BOOT_RESAMPLE_MS) {
      batterySampleNow();
      lastResample = millis();
    }
    const BatteryData& b = batteryGetData();
    int target = b.valid ? b.percent : 0;
    int dispPct;
    if (BOOT_CHARGE_PHASE_MS == 0)
      dispPct = target;
    else
      dispPct = (int)((el * (long)target) / (long)BOOT_CHARGE_PHASE_MS);
    if (dispPct > target) dispPct = target;
    if (dispPct < 0) dispPct = 0;
    drawBootChargeScreen(dispPct, b);
    delay(BOOT_FRAME_MS);
    yield();
  }

  batterySampleNow();
  {
    unsigned long tHold = millis();
    while (millis() - tHold < BOOT_HOLD_PHASE_MS) {
      if (millis() - lastResample >= BOOT_RESAMPLE_MS) {
        batterySampleNow();
        lastResample = millis();
      }
      drawBootHoldScreen(batteryGetData());
      delay(BOOT_FRAME_MS);
      yield();
    }
  }

  {
    unsigned long tWake = millis();
    while (millis() - tWake < BOOT_WAKE_PHASE_MS) {
      uint32_t wel = (uint32_t)(millis() - tWake);
      drawBootWakeScreen(wel);
      delay(BOOT_FRAME_MS);
      yield();
    }
  }

  batterySetVerboseSampleLog(true);
  batterySampleNow();
  Serial.println(F("[Display] Boot sequence complete -> main panel"));
}

static void drawStringDatum(Adafruit_GFX* gfx, int16_t x, int16_t y, const char* s, int datum) {
  int16_t x1, y1;
  uint16_t tw, th;
  gfx->getTextBounds(s, 0, 0, &x1, &y1, &tw, &th);
  if (datum == DATUM_TR)  { x -= tw; }
  else if (datum == DATUM_BC || datum == DATUM_MC) { x -= tw / 2; }
  if (datum == DATUM_BC)  { y -= th; }
  else if (datum == DATUM_MC) { y -= th / 2; }
  gfx->setCursor(x, y);
  gfx->print(s);
}

// ============================================================
//  Static UI
// ============================================================
static void drawHeader() {
  tft->fillRect(0, HDR_Y, W, HDR_H, C_SURFACE);
  tft->drawFastHLine(0, HDR_Y + HDR_H - 1, W, C_BORDER);
  tft->setTextColor(C_ACCENT, C_SURFACE);
  tft->setTextSize(2);
  drawStringDatum(tft, W / 2, HDR_Y + 18, "WALL-E", DATUM_MC);
}

static void drawSectionLabel(const char* label, int y) {
  tft->setTextSize(1);
  tft->setTextColor(C_TXT_DIM, C_BG);
  tft->setCursor(12, y);
  tft->print(label);
}

// ============================================================
//  Command panel
// ============================================================
struct CmdStyle {
  const char* label;
  uint16_t    bg, fg;
  int8_t      jx, jy;  // joystick stick offset from center (-1,0,1 or 0,0 for center)
};

static CmdStyle getCmdStyle(DriveCommand cmd) {
  switch (cmd) {
    case CMD_FORWARD: return { "FWD",  C_SURFACE2, C_ACCENT,    0, -1 };
    case CMD_REVERSE: return { "REV",  C_SURFACE2, C_TXT_DIM,   0,  1 };
    case CMD_LEFT:    return { "LEFT", C_SURFACE2, C_ACCENT,  -1,  0 };
    case CMD_RIGHT:   return { "RIGHT",C_SURFACE2, C_ACCENT,   1,  0 };
    case CMD_STOP:    return { "STOP", 0x3000,     C_STOP,     0,  0 };
    case CMD_DRIVE:   return { "DRIVE",C_SURFACE2, C_ACCENT,   0,  0 };  // stick uses _joyX, _joyY
    default:          return { "IDLE", C_SURFACE2, C_TXT_DIM,  0,  0 };
  }
}

// Joystick: base circle + stick nub; offset (sx,sy) in pixels from center for smooth tank display
static void drawJoystickSmooth(GFXcanvas16* spr, int cx, int cy, float jx, float jy, uint16_t stickCol, uint16_t baseCol) {
  const int baseR = 26;
  const int stickR = 10;
  const int stickOffset = 14;
  jx = constrain(jx, -1.0f, 1.0f);
  jy = constrain(jy, -1.0f, 1.0f);
  spr->drawCircle(cx, cy, baseR, C_BORDER);
  spr->fillCircle(cx, cy, baseR - 2, baseCol);
  int sx = cx + (int)(jx * stickOffset);
  int sy = cy + (int)(jy * stickOffset);
  spr->fillCircle(sx, sy, stickR, stickCol);
  spr->drawCircle(sx, sy, stickR, C_BORDER);
}

static void redrawCommand() {
  CmdStyle s = getCmdStyle(_cmd);
  int sh = CMD_H;
  int cx = SPR_W / 2;
  int cy = sh / 2 - 12;
  sprCmd->fillScreen(C_BG);
  sprCmd->fillRoundRect(0, 0, SPR_W, sh, 10, s.bg);
  sprCmd->drawRoundRect(0, 0, SPR_W, sh, 10, C_BORDER);
  drawJoystickSmooth(sprCmd, cx, cy, _joyX, _joyY, s.fg, s.bg);
  sprCmd->setTextColor(s.fg, s.bg);
  sprCmd->setTextSize(2);
  drawStringDatum(sprCmd, SPR_W / 2, sh - 10, s.label, DATUM_BC);
  tft->drawRGBBitmap(12, CMD_Y, sprCmd->getBuffer(), SPR_W, sh);
}

// ============================================================
//  Speed panel
// ============================================================
static void redrawSpeed() {
  int sh = SPD_H;
  sprSpd->fillScreen(C_BG);
  sprSpd->fillRoundRect(0, 0, SPR_W, sh, 8, C_SURFACE2);
  sprSpd->drawRoundRect(0, 0, SPR_W, sh, 8, C_BORDER);
  sprSpd->setTextSize(1);
  sprSpd->setTextColor(C_TXT_DIM, C_SURFACE2);
  sprSpd->setCursor(10, 6);
  sprSpd->print("SPEED");
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", _speed);
  sprSpd->setTextColor(C_ACCENT, C_SURFACE2);
  drawStringDatum(sprSpd, SPR_W - 10, 5, buf, DATUM_TR);
  int barX = 10, barY = 22, barW = SPR_W - 20, barH = 7;
  int fillW = (int)((float)_speed / 255.0f * barW);
  sprSpd->fillRoundRect(barX, barY, barW, barH, 3, C_BORDER);
  uint16_t barCol = (_speed < 100) ? C_OK : (_speed < 200) ? C_ACCENT : C_STOP;
  if (fillW > 0) sprSpd->fillRoundRect(barX, barY, fillW, barH, 4, barCol);
  tft->drawRGBBitmap(12, SPD_Y, sprSpd->getBuffer(), SPR_W, sh);
}

// ============================================================
//  WiFi panel
// ============================================================
static void redrawWifi() {
  int sh = WIFI_H;
  sprWifi->fillScreen(C_BG);
  sprWifi->fillRoundRect(0, 0, SPR_W, sh, 8, C_SURFACE2);
  sprWifi->drawRoundRect(0, 0, SPR_W, sh, 8, C_BORDER);
  WiFiState state = wifiGetState();
  sprWifi->setTextSize(1);
  sprWifi->setTextColor(C_TXT_DIM, C_SURFACE2);
  sprWifi->setCursor(10, 5);
  sprWifi->print("AP");
  sprWifi->setTextColor(C_WARN, C_SURFACE2);
  drawStringDatum(sprWifi, SPR_W - 10, 5, wifiGetAP_IP().c_str(), DATUM_TR);
  sprWifi->drawFastHLine(10, 17, SPR_W - 20, C_BORDER);
  sprWifi->setTextColor(C_TXT_DIM, C_SURFACE2);
  sprWifi->setCursor(10, 19);
  sprWifi->print("STA");
  String staVal;
  uint16_t staCol;
  switch (state) {
    case WS_CONNECTED:  staVal = wifiGetSTA_IP();       staCol = C_OK;   break;
    case WS_CONNECTING: staVal = "Connecting...";      staCol = C_WARN; break;
    case WS_FAILED:    staVal = "Failed";              staCol = C_STOP; break;
    default:            staVal = "Not connected";      staCol = C_TXT_DIM; break;
  }
  sprWifi->setTextColor(staCol, C_SURFACE2);
  drawStringDatum(sprWifi, SPR_W - 10, 19, staVal.c_str(), DATUM_TR);
  tft->drawRGBBitmap(12, WIFI_Y, sprWifi->getBuffer(), SPR_W, sh);
}

// ============================================================
//  Battery panel
// ============================================================
static void redrawBattery() {
  const BatteryData& bat = batteryGetData();
  int sh = BAT_H;
  sprBat->fillScreen(C_BG);
  sprBat->fillRoundRect(0, 0, SPR_W, sh, 6, C_SURFACE2);
  sprBat->drawRoundRect(0, 0, SPR_W, sh, 6, C_BORDER);
  sprBat->setTextSize(1);
  sprBat->setTextColor(C_TXT_DIM, C_SURFACE2);
  sprBat->setCursor(10, 6);
  sprBat->print("BATTERY");
  if (!bat.valid) {
    sprBat->setTextColor(C_TXT_DIM, C_SURFACE2);
    drawStringDatum(sprBat, SPR_W - 10, 6, "N/A", DATUM_TR);
    tft->drawRGBBitmap(12, BAT_Y, sprBat->getBuffer(), SPR_W, sh);
    return;
  }
  uint16_t barCol = (bat.status == BAT_CRITICAL) ? C_STOP : (bat.status == BAT_WARNING) ? C_WARN : C_OK;
  char buf[24];
  snprintf(buf, sizeof(buf), "%.2fV %.2fA %d%%", bat.voltage, bat.currentA, bat.percent);
  sprBat->setTextColor(barCol, C_SURFACE2);
  drawStringDatum(sprBat, SPR_W - 10, 6, buf, DATUM_TR);
  int barX = 10, barY = 20, barW = SPR_W - 20, barH = 6;
  int fillW = (bat.percent * barW) / 100;
  sprBat->fillRoundRect(barX, barY, barW, barH, 3, C_BORDER);
  if (fillW > 0) sprBat->fillRoundRect(barX, barY, fillW, barH, 3, barCol);
  tft->drawRGBBitmap(12, BAT_Y, sprBat->getBuffer(), SPR_W, sh);
}

static void drawNormalUILayout(void) {
  tft->fillScreen(C_BG);
  drawHeader();
  drawSectionLabel("DRIVE", 38);
  drawSectionLabel("SPEED", 118);
  drawSectionLabel("BATTERY", 160);
  drawSectionLabel("NETWORK", 200);
  redrawCommand();
  redrawSpeed();
  redrawBattery();
  redrawWifi();
}

// ============================================================
//  Public API
// ============================================================
void displayInit() {
  // Same init sequence as working display_control.cpp
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  SPI.setFrequency(TFT_SPI_FREQ);

  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(200);
  digitalWrite(TFT_RST, HIGH);
  delay(200);

  tft = new Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
  tft->init(240, 240, SPI_MODE3);
  tft->setRotation(0);
  tft->fillScreen(C_BLACK);

  // Create offscreen canvases for the four panels (used after boot)
  sprCmd  = new GFXcanvas16(SPR_W, CMD_H);
  sprSpd  = new GFXcanvas16(SPR_W, SPD_H);
  sprBat  = new GFXcanvas16(SPR_W, BAT_H);
  sprWifi = new GFXcanvas16(SPR_W, WIFI_H);

  // Dim backlight during boot readout, then full after sequence
  ledcAttach(TFT_BL, BLK_PWM_FREQ, BLK_PWM_RES);
  ledcWrite(TFT_BL, 48);
  delay(30);

  displayRunPowerOnSequence();

  for (int i = 48; i <= BLK_BRIGHTNESS; i += 8) {
    ledcWrite(TFT_BL, i);
    delay(6);
  }

  drawNormalUILayout();

  Serial.println("[Display] ST7789 240x240 initialised (Adafruit)");
}

void displaySetCommand(DriveCommand cmd) {
  if (_cmd == cmd) return;
  _cmd = cmd;
  CmdStyle s = getCmdStyle(cmd);
  _joyX = (float)s.jx;
  _joyY = (float)s.jy;
  _cmdDirty = true;
}

void displaySetSpeed(uint8_t speed) {
  if (_speed == speed) return;
  _speed = speed;
  _spdDirty = true;
}

void displaySetStick(float jx, float jy) {
  _joyX = constrain(jx, -1.0f, 1.0f);
  _joyY = constrain(jy, -1.0f, 1.0f);
  _cmd = (fabsf(_joyX) < 0.05f && fabsf(_joyY) < 0.05f) ? CMD_IDLE : CMD_DRIVE;
  _cmdDirty = true;
}

void displayUpdateWifi()  { _wifiDirty = true; }
void displayUpdateBattery() { _batDirty = true; }

void displayShowToast(const char* msg) {
  if (!msg) return;
  strncpy(_toastBuf, msg, sizeof(_toastBuf) - 1);
  _toastBuf[sizeof(_toastBuf) - 1] = '\0';
  _toastUntil = millis() + 2000;
  _cmdDirty = true;  /* Trigger redraw so toast visible */
}

static void drawToast() {
  if (_toastUntil == 0 || millis() >= _toastUntil) return;
  if (_toastBuf[0] == '\0') return;
  int th = 24;
  int ty = H - th - 8;
  tft->fillRoundRect(12, ty, SPR_W, th, 6, C_SURFACE2);
  tft->drawRoundRect(12, ty, SPR_W, th, 6, C_BORDER);
  tft->setTextColor(C_TXT, C_SURFACE2);
  tft->setTextSize(1);
  int16_t x1, y1;
  uint16_t tw, th2;
  tft->getTextBounds(_toastBuf, 0, 0, &x1, &y1, &tw, &th2);
  tft->setCursor(12 + (SPR_W - tw) / 2, ty + (th - th2) / 2);
  tft->print(_toastBuf);
}

void displayHandle() {
  if (!_cmdDirty && !_spdDirty && !_batDirty && !_wifiDirty) return;
  if ((millis() - _lastDraw) < DRAW_INTERVAL_MS) return;
  _lastDraw = millis();

  if (_cmdDirty)  { redrawCommand();  _cmdDirty  = false; }
  if (_spdDirty)  { redrawSpeed();    _spdDirty  = false; }
  if (_batDirty)  { redrawBattery();  _batDirty  = false; }
  if (_wifiDirty) { redrawWifi();     _wifiDirty = false; }
  drawToast();
}
