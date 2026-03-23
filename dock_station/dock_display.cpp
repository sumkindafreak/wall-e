/*******************************************************************************
 * dock_display.cpp
 * 1.8" TFT SPI 128x160 (ST7735) — CYD-style dock status UI (no touch)
 * Same look as WALL-E Master Controller: industrial graphite + amber, top bar, panel.
 ******************************************************************************/

#include "dock_display.h"
#include "dock_config.h"
#include "dock_state.h"
#include "dock_sensors.h"
#include "dock_hw.h"
#include "dock_protocol.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Arduino.h>

#define TFT_WIDTH   128
#define TFT_HEIGHT  160

/* CYD-style colors (match wall_e_master_controller/ui_draw.h) — RGB565 */
#define C_BG        0x0000
#define C_BG_DARK   0x18C3
#define C_BORDER    0x3186
#define C_ACCENT    0xFD20
#define C_ACCENT_DIM 0xB360
#define C_RED       0xF800
#define C_GREEN     0x07E0
#define C_WHITE     0xFFFF
#define C_TEXT_DIM  0xAD55

/* Layout — CYD-like top bar + content panel */
#define TOP_BAR_H     20
#define TOP_BAR_STATUS_X  74   /* X for "DOCKED" / "IDLE" after "WALL-E Dock " */
#define PANEL_LEFT    4
#define PANEL_TOP     22
#define PANEL_W       (TFT_WIDTH - 2 * PANEL_LEFT)
#define PANEL_H       (TFT_HEIGHT - PANEL_TOP - 4)
#define LINE_H        10
#define LINE_W        (PANEL_W - 8)
#define ROW0_Y        (PANEL_TOP + 4)
#define ROW1_Y        (ROW0_Y + LINE_H)
#define ROW2_Y        (ROW1_Y + LINE_H)
#define ROW3_Y        (ROW2_Y + LINE_H)
#define ROW4_Y        (ROW3_Y + LINE_H)
#define ROW5_Y        (ROW4_Y + LINE_H)
#define ROW6_Y        (ROW5_Y + LINE_H)
#define ROW7_Y        (ROW6_Y + LINE_H)
#define ROW8_Y        (ROW7_Y + LINE_H)

static Adafruit_ST7735* tft = nullptr;
static uint32_t g_last_update = 0;
#define UPDATE_MS  250

static DockState g_last_s = STATE_BOOT;
static bool g_last_idle = false;
static int16_t g_last_i100 = 0;
static bool g_last_charge = false;
static bool g_last_beam = false;
static bool g_last_o[4] = {false, false, false, false};  /* FL, FR, BL, BR */
static bool g_first_draw = true;
static bool g_last_docked = false;  /* for top bar DOCKED / IDLE */

static void drawTopBarStatus(bool docked) {
  tft->fillRect(TOP_BAR_STATUS_X, 2, TFT_WIDTH - TOP_BAR_STATUS_X - 2, TOP_BAR_H - 4, C_BG_DARK);
  tft->setCursor(TOP_BAR_STATUS_X, 6);
  tft->setTextSize(1);
  tft->setTextColor(docked ? C_GREEN : C_TEXT_DIM, C_BG_DARK);
  tft->print(docked ? F("DOCKED") : F("IDLE"));
}

/* Line 0: STATE = (current state) */
static void drawLine0(void) {
  tft->fillRect(PANEL_LEFT + 2, ROW0_Y - 2, LINE_W + 4, LINE_H + 2, C_BG_DARK);
  tft->setCursor(PANEL_LEFT + 4, ROW0_Y);
  tft->setTextSize(1);
  tft->setTextColor(C_ACCENT, C_BG_DARK);
  tft->print(F("STATE = "));
  tft->setTextColor(C_WHITE, C_BG_DARK);
  if (g_last_idle) {
    tft->print(F("IDLE"));
  } else {
    switch (g_last_s) {
      case STATE_BOOT:        tft->print(F("BOOT")); break;
      case STATE_NOT_DOCKED:  tft->print(F("IDLE")); break;
      case STATE_DOCKED_IDLE: tft->print(F("DOCKED")); break;
      case STATE_CHARGING:    tft->print(F("CHARGING")); break;
      case STATE_CHARGED:     tft->print(F("CHARGED")); break;
      case STATE_FAULT:
        tft->setTextColor(C_RED, C_BG_DARK);
        tft->print(dockStateGetFaultCode() == FAULT_OVERCURRENT ? F("FAULT OVC") : F("FAULT OFF"));
        tft->setTextColor(C_WHITE, C_BG_DARK);
        break;
      default: tft->print(F("?")); break;
    }
  }
}

/* Line 1: CHARGE = ENABLED / DISABLED */
static void drawLine1(void) {
  tft->fillRect(PANEL_LEFT + 2, ROW1_Y - 2, LINE_W + 4, LINE_H + 2, C_BG_DARK);
  tft->setCursor(PANEL_LEFT + 4, ROW1_Y);
  tft->setTextSize(1);
  tft->setTextColor(C_ACCENT, C_BG_DARK);
  tft->print(F("CHARGE = "));
  tft->setTextColor(g_last_charge ? C_GREEN : C_TEXT_DIM, C_BG_DARK);
  tft->print(g_last_charge ? F("ENABLED") : F("DISABLED"));
}

/* Line 2: Amps = X.XX A  or  Amps = N/A (no current sense) */
static void drawLine2(void) {
  tft->fillRect(PANEL_LEFT + 2, ROW2_Y - 2, LINE_W + 4, LINE_H + 2, C_BG_DARK);
  tft->setCursor(PANEL_LEFT + 4, ROW2_Y);
  tft->setTextSize(1);
  tft->setTextColor(C_ACCENT, C_BG_DARK);
  tft->print(F("Amps = "));
  tft->setTextColor(C_WHITE, C_BG_DARK);
  if (dockCurrentSenseAvailable()) {
    tft->print(g_last_i100 / 100.0f, 2);
    tft->print(F(" A"));
  } else {
    tft->print(F("N/A"));
  }
}

/* Line 3: Dock = YES / no (logical dock-detected, beam removed) */
static void drawLine3(void) {
  tft->fillRect(PANEL_LEFT + 2, ROW3_Y - 2, LINE_W + 4, LINE_H + 2, C_BG_DARK);
  tft->setCursor(PANEL_LEFT + 4, ROW3_Y);
  tft->setTextSize(1);
  tft->setTextColor(C_ACCENT, C_BG_DARK);
  tft->print(F("Dock = "));
  tft->setTextColor(g_last_beam ? C_GREEN : C_TEXT_DIM, C_BG_DARK);
  tft->print(g_last_beam ? F("YES") : F("no"));
}

/* Lines 4–7: Obstacles FL, FR, BL, BR */
static void drawObstacleLine(int index) {
  int y = (index == 0) ? ROW4_Y : (index == 1) ? ROW5_Y : (index == 2) ? ROW6_Y : ROW7_Y;
  tft->fillRect(PANEL_LEFT + 2, y - 2, LINE_W + 4, LINE_H + 2, C_BG_DARK);
  tft->setCursor(PANEL_LEFT + 4, y);
  tft->setTextSize(1);
  tft->setTextColor(C_ACCENT, C_BG_DARK);
  if (index == 0) tft->print(F("Obst FL = "));
  else if (index == 1) tft->print(F("Obst FR = "));
  else if (index == 2) tft->print(F("Obst BL = "));
  else tft->print(F("Obst BR = "));
  tft->setTextColor(g_last_o[index] ? C_RED : C_GREEN, C_BG_DARK);
  tft->print(g_last_o[index] ? F("BLOCKED") : F("CLEAR"));
}

/* Line 8: Station #1 — this dock’s ID (for multi-dock / ESP-NOW). Was "Dock 0x1". */
static void drawLine8(void) {
  tft->fillRect(PANEL_LEFT + 2, ROW8_Y - 2, LINE_W + 4, LINE_H + 2, C_BG_DARK);
  tft->setCursor(PANEL_LEFT + 4, ROW8_Y);
  tft->setTextSize(1);
  tft->setTextColor(C_TEXT_DIM, C_BG_DARK);
  tft->print(F("Station #"));
  tft->print((unsigned long)DOCK_ID);
}

void dockDisplayBegin(void) {
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  SPI.begin(PIN_TFT_SCK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
  tft = new Adafruit_ST7735(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
  tft->initR(INITR_GREENTAB);
  tft->setRotation(0);
  tft->fillScreen(C_BG);

  /* CYD-style top bar: "WALL-E Dock" + DOCKED (green) / IDLE (dim when robot left) */
  tft->fillRect(0, 0, TFT_WIDTH, TOP_BAR_H, C_BG_DARK);
  tft->drawFastHLine(0, TOP_BAR_H - 1, TFT_WIDTH, C_BORDER);
  tft->setTextColor(C_WHITE, C_BG_DARK);
  tft->setTextSize(1);
  tft->setCursor(6, 6);
  tft->print(F("WALL-E Dock "));
  /* Top bar DOCKED = state-based so it stays green for DOCKED_IDLE / CHARGING / CHARGED */
  {
    DockState s0 = dockStateGet();
    g_last_docked = (s0 == STATE_DOCKED_IDLE || s0 == STATE_CHARGING || s0 == STATE_CHARGED);
  }
  drawTopBarStatus(g_last_docked);

  /* Content panel: bordered area like CYD */
  tft->fillRect(PANEL_LEFT, PANEL_TOP, PANEL_W, PANEL_H, C_BG_DARK);
  tft->drawRect(PANEL_LEFT, PANEL_TOP, PANEL_W, PANEL_H, C_BORDER);
  tft->setTextWrap(false);
  g_first_draw = true;
}

void dockDisplayUpdate(void) {
  if (!tft) return;
  uint32_t now = millis();

  DockState s = dockStateGet();
  float i = dockCurrentAmps();
  bool beam = dockDockDetected();
  bool idle_mode = dockIsIdleMode();
  bool charge = dockChargeEnabled();
  int16_t i100 = (int16_t)(i * 100);

  /* First draw: show all lines immediately (no rate limit) */
  if (g_first_draw) {
    g_first_draw = false;
    g_last_update = now;
    g_last_s = s;
    g_last_idle = idle_mode;
    g_last_i100 = i100;
    g_last_charge = charge;
    g_last_beam = beam;
    g_last_docked = (s == STATE_DOCKED_IDLE || s == STATE_CHARGING || s == STATE_CHARGED);
    for (int n = 0; n < 4; n++) g_last_o[n] = dockObstacleBlocked(n);
    drawLine0();
    drawLine1();
    drawLine2();
    drawLine3();
    for (int n = 0; n < 4; n++) drawObstacleLine(n);
    drawLine8();
    return;
  }

  if (now - g_last_update < UPDATE_MS) return;
  g_last_update = now;

  /* Top bar: DOCKED (green) when state is DOCKED_IDLE, CHARGING, or CHARGED */
  bool docked = (s == STATE_DOCKED_IDLE || s == STATE_CHARGING || s == STATE_CHARGED);
  if (g_last_docked != docked) {
    g_last_docked = docked;
    drawTopBarStatus(g_last_docked);
  }

  if (g_last_s != s || g_last_idle != idle_mode) {
    g_last_s = s;
    g_last_idle = idle_mode;
    drawLine0();
  }
  if (g_last_charge != charge) {
    g_last_charge = charge;
    drawLine1();
  }
  if (g_last_i100 != i100) {
    g_last_i100 = i100;
    drawLine2();
  }
  if (g_last_beam != beam) {
    g_last_beam = beam;
    drawLine3();
  }
  for (int n = 0; n < 4; n++) {
    bool o = dockObstacleBlocked(n);
    if (g_last_o[n] != o) {
      g_last_o[n] = o;
      drawObstacleLine(n);
    }
  }
  /* Station # drawn once on first draw */
}
