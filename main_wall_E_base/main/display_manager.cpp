// ============================================================
// WALL-E Base display manager
// ============================================================

#include "display_manager.h"
#include "wifi_manager.h"
#include "battery_monitor.h"
#include <Arduino.h>
#include <cmath>
#include <cstring>

#if !WALLE_BASE_LOCAL_TFT

// ============================================================
// ESP32-P4 headless backend
//
// The Base remains fully functional without consuming five GPIOs for the old
// ST7789. CYD and LROS are the operator displays. State is still retained so a
// future DSI backend can drop in behind this same API.
// ============================================================

static DriveCommand s_cmd = CMD_IDLE;
static uint8_t s_speed = 0;
static float s_stickX = 0.0f;
static float s_stickY = 0.0f;
static char s_lastToast[48] = "";

void displayInit() {
  Serial.printf("[Display] Local TFT disabled on %s; using CYD/LROS status UI\n",
                WALLE_BASE_BOARD_NAME);
}

void displaySetCommand(DriveCommand cmd) {
  s_cmd = cmd;
}

void displaySetSpeed(uint8_t speed) {
  s_speed = speed;
}

void displaySetStick(float jx, float jy) {
  s_stickX = constrain(jx, -1.0f, 1.0f);
  s_stickY = constrain(jy, -1.0f, 1.0f);
  (void)s_stickX;
  (void)s_stickY;
}

void displayUpdateWifi() {
  // Web/CYD status reads the network service directly.
}

void displayUpdateBattery() {
  // Web/CYD status reads battery_monitor directly.
}

void displayShowToast(const char* msg) {
  if (!msg) return;
  strncpy(s_lastToast, msg, sizeof(s_lastToast) - 1);
  s_lastToast[sizeof(s_lastToast) - 1] = '\0';
  Serial.printf("[Display/Toast] %s\n", s_lastToast);
}

void displayHandle() {
  // Intentionally empty on the headless P4 Base.
  (void)s_cmd;
  (void)s_speed;
}

#else

// ============================================================
// Legacy ST7789 backend
// ============================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define C_BG        0x0861
#define C_SURFACE   0x0CA3
#define C_SURFACE2  0x0D04
#define C_BORDER    0x1284
#define C_ACCENT    0xFB04
#define C_TXT       0xDEF7
#define C_TXT_DIM   0x2B10
#define C_OK        0x1F8A
#define C_STOP      0xE189
#define C_WARN      0xFB04
#define C_BLACK     0x0000
#define C_WHITE     0xFFFF

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

static Adafruit_ST7789* tft = nullptr;
static GFXcanvas16* sprCmd  = nullptr;
static GFXcanvas16* sprSpd  = nullptr;
static GFXcanvas16* sprBat  = nullptr;
static GFXcanvas16* sprWifi = nullptr;

static const int SPR_W = W - 24;

static DriveCommand _cmd   = CMD_IDLE;
static uint8_t _speed = 200;
static float _joyX = 0;
static float _joyY = 0;
static bool _cmdDirty = true;
static bool _spdDirty = true;
static bool _batDirty = true;
static bool _wifiDirty = true;
static unsigned long _lastDraw = 0;
#define DRAW_INTERVAL_MS 80

static char _toastBuf[48] = "";
static unsigned long _toastUntil = 0;

enum { DATUM_TL, DATUM_TR, DATUM_BC, DATUM_MC };

static void drawStringDatum(Adafruit_GFX* gfx, int16_t x, int16_t y,
                            const char* s, int datum) {
  int16_t x1, y1;
  uint16_t tw, th;
  gfx->getTextBounds(s, 0, 0, &x1, &y1, &tw, &th);
  if (datum == DATUM_TR) x -= tw;
  else if (datum == DATUM_BC || datum == DATUM_MC) x -= tw / 2;
  if (datum == DATUM_BC) y -= th;
  else if (datum == DATUM_MC) y -= th / 2;
  gfx->setCursor(x, y);
  gfx->print(s);
}

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

struct CmdStyle {
  const char* label;
  uint16_t bg;
  uint16_t fg;
  int8_t jx;
  int8_t jy;
};

static CmdStyle getCmdStyle(DriveCommand cmd) {
  switch (cmd) {
    case CMD_FORWARD: return {"FWD", C_SURFACE2, C_ACCENT, 0, -1};
    case CMD_REVERSE: return {"REV", C_SURFACE2, C_TXT_DIM, 0, 1};
    case CMD_LEFT:    return {"LEFT", C_SURFACE2, C_ACCENT, -1, 0};
    case CMD_RIGHT:   return {"RIGHT", C_SURFACE2, C_ACCENT, 1, 0};
    case CMD_STOP:    return {"STOP", 0x3000, C_STOP, 0, 0};
    case CMD_DRIVE:   return {"DRIVE", C_SURFACE2, C_ACCENT, 0, 0};
    default:          return {"IDLE", C_SURFACE2, C_TXT_DIM, 0, 0};
  }
}

static void drawJoystickSmooth(GFXcanvas16* spr, int cx, int cy,
                               float jx, float jy,
                               uint16_t stickCol, uint16_t baseCol) {
  const int baseR = 26;
  const int stickR = 10;
  const int stickOffset = 14;
  jx = constrain(jx, -1.0f, 1.0f);
  jy = constrain(jy, -1.0f, 1.0f);
  spr->drawCircle(cx, cy, baseR, C_BORDER);
  spr->fillCircle(cx, cy, baseR - 2, baseCol);
  const int sx = cx + (int)(jx * stickOffset);
  const int sy = cy + (int)(jy * stickOffset);
  spr->fillCircle(sx, sy, stickR, stickCol);
  spr->drawCircle(sx, sy, stickR, C_BORDER);
}

static void redrawCommand() {
  const CmdStyle s = getCmdStyle(_cmd);
  const int sh = CMD_H;
  const int cx = SPR_W / 2;
  const int cy = sh / 2 - 12;
  sprCmd->fillScreen(C_BG);
  sprCmd->fillRoundRect(0, 0, SPR_W, sh, 10, s.bg);
  sprCmd->drawRoundRect(0, 0, SPR_W, sh, 10, C_BORDER);
  drawJoystickSmooth(sprCmd, cx, cy, _joyX, _joyY, s.fg, s.bg);
  sprCmd->setTextColor(s.fg, s.bg);
  sprCmd->setTextSize(2);
  drawStringDatum(sprCmd, SPR_W / 2, sh - 10, s.label, DATUM_BC);
  tft->drawRGBBitmap(12, CMD_Y, sprCmd->getBuffer(), SPR_W, sh);
}

static void redrawSpeed() {
  const int sh = SPD_H;
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
  const int barX = 10;
  const int barY = 22;
  const int barW = SPR_W - 20;
  const int barH = 7;
  const int fillW = (int)((float)_speed / 255.0f * barW);
  sprSpd->fillRoundRect(barX, barY, barW, barH, 3, C_BORDER);
  const uint16_t barCol = (_speed < 100) ? C_OK :
                          (_speed < 200) ? C_ACCENT : C_STOP;
  if (fillW > 0) sprSpd->fillRoundRect(barX, barY, fillW, barH, 4, barCol);
  tft->drawRGBBitmap(12, SPD_Y, sprSpd->getBuffer(), SPR_W, sh);
}

static void redrawWifi() {
  const int sh = WIFI_H;
  sprWifi->fillScreen(C_BG);
  sprWifi->fillRoundRect(0, 0, SPR_W, sh, 8, C_SURFACE2);
  sprWifi->drawRoundRect(0, 0, SPR_W, sh, 8, C_BORDER);
  const WiFiState state = wifiGetState();
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
    case WS_CONNECTED:
      staVal = wifiGetSTA_IP();
      staCol = C_OK;
      break;
    case WS_CONNECTING:
      staVal = "Connecting...";
      staCol = C_WARN;
      break;
    case WS_FAILED:
      staVal = "Failed";
      staCol = C_STOP;
      break;
    default:
      staVal = "Not connected";
      staCol = C_TXT_DIM;
      break;
  }
  sprWifi->setTextColor(staCol, C_SURFACE2);
  drawStringDatum(sprWifi, SPR_W - 10, 19, staVal.c_str(), DATUM_TR);
  tft->drawRGBBitmap(12, WIFI_Y, sprWifi->getBuffer(), SPR_W, sh);
}

static void redrawBattery() {
  const BatteryData& bat = batteryGetData();
  const int sh = BAT_H;
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

  const uint16_t barCol = (bat.status == BAT_CRITICAL) ? C_STOP :
                          (bat.status == BAT_WARNING) ? C_WARN : C_OK;
  char buf[24];
  snprintf(buf, sizeof(buf), "%.2fV %.2fA %d%%",
           bat.voltage, bat.currentA, bat.percent);
  sprBat->setTextColor(barCol, C_SURFACE2);
  drawStringDatum(sprBat, SPR_W - 10, 6, buf, DATUM_TR);
  const int barX = 10;
  const int barY = 20;
  const int barW = SPR_W - 20;
  const int barH = 6;
  const int fillW = (bat.percent * barW) / 100;
  sprBat->fillRoundRect(barX, barY, barW, barH, 3, C_BORDER);
  if (fillW > 0) sprBat->fillRoundRect(barX, barY, fillW, barH, 3, barCol);
  tft->drawRGBBitmap(12, BAT_Y, sprBat->getBuffer(), SPR_W, sh);
}

void displayInit() {
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
  tft->fillScreen(C_BG);

  sprCmd = new GFXcanvas16(SPR_W, CMD_H);
  sprSpd = new GFXcanvas16(SPR_W, SPD_H);
  sprBat = new GFXcanvas16(SPR_W, BAT_H);
  sprWifi = new GFXcanvas16(SPR_W, WIFI_H);

  drawHeader();
  drawSectionLabel("DRIVE", 38);
  drawSectionLabel("SPEED", 118);
  drawSectionLabel("BATTERY", 160);
  drawSectionLabel("NETWORK", 200);

  redrawCommand();
  redrawSpeed();
  redrawBattery();
  redrawWifi();

  ledcAttach(TFT_BL, BLK_PWM_FREQ, BLK_PWM_RES);
  for (int i = 0; i <= BLK_BRIGHTNESS; i += 5) {
    ledcWrite(TFT_BL, i);
    delay(8);
  }

  Serial.println("[Display] ST7789 240x240 initialised (Adafruit)");
}

void displaySetCommand(DriveCommand cmd) {
  if (_cmd == cmd) return;
  _cmd = cmd;
  const CmdStyle s = getCmdStyle(cmd);
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
  _cmd = (fabsf(_joyX) < 0.05f && fabsf(_joyY) < 0.05f)
             ? CMD_IDLE
             : CMD_DRIVE;
  _cmdDirty = true;
}

void displayUpdateWifi() {
  _wifiDirty = true;
}

void displayUpdateBattery() {
  _batDirty = true;
}

void displayShowToast(const char* msg) {
  if (!msg) return;
  strncpy(_toastBuf, msg, sizeof(_toastBuf) - 1);
  _toastBuf[sizeof(_toastBuf) - 1] = '\0';
  _toastUntil = millis() + 2000;
  _cmdDirty = true;
}

static void drawToast() {
  if (_toastUntil == 0 || millis() >= _toastUntil || _toastBuf[0] == '\0') {
    return;
  }

  const int th = 24;
  const int ty = H - th - 8;
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

  if (_cmdDirty) {
    redrawCommand();
    _cmdDirty = false;
  }
  if (_spdDirty) {
    redrawSpeed();
    _spdDirty = false;
  }
  if (_batDirty) {
    redrawBattery();
    _batDirty = false;
  }
  if (_wifiDirty) {
    redrawWifi();
    _wifiDirty = false;
  }
  drawToast();
}

#endif  // WALLE_BASE_LOCAL_TFT
