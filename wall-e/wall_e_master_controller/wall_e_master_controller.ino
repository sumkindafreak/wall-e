// ============================================================
//  WALL-E Master Controller — CYD Direct-Draw UI
//  TFT_eSPI + XPT2046, no LVGL
//  State machine, animations, audio, zero-flicker
// ============================================================
//
// REQUIRED: TFT_eSPI (Setup_CYD_ESP32_2432S028R), XPT2046_Touchscreen
//
// ============================================================

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "protocol.h"
#include "ui_draw.h"
#include "ui_state.h"
#include "touch_input.h"
#include "animation_system.h"
#include "audio_system.h"
#include "packet_control.h"
#include "espnow_control.h"

#define TFT_BL 21

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[WALL-E Master] CYD Command Console");

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);

  uiDrawInit(&tft);
  uiStateInit();
  touchInit();
  animInit();
  audioInit();
  packetInit();

  g_needStaticRedraw = true;

  Serial.println("[WALL-E Master] Ready");
}

void loop() {
  unsigned long now = millis();

  int pageInt = (int)g_currentPage;
  TouchZone zone = touchUpdate(pageInt);

  DriveState ds;
  touchGetDriveState(&ds);

  // Safety lock: no touch for 200ms → STOP
  if (now - touchLastActivityMs() > PACKET_SAFETY_TIMEOUT_MS) {
    ds.leftSpeed = 0;
    ds.rightSpeed = 0;
  }

  // Zone actions
  if (zone == TOUCH_ZONE_ESTOP) {
    g_estop = true;
    ds.leftSpeed = 0;
    ds.rightSpeed = 0;
    g_controlAuthority = CTRL_SAFETY;
    playUISound(SOUND_ESTOP);
  }
  if (zone == TOUCH_ZONE_NAV_BEHAV) {
    g_currentPage = PAGE_BEHAVIOUR;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_SYSTEM) {
    g_currentPage = PAGE_SYSTEM;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_NAV_BACK) {
    g_currentPage = PAGE_DRIVE;
    g_needStaticRedraw = true;
    playUISound(SOUND_CLICK);
  }
  if (zone == TOUCH_ZONE_QUICK_ACTION) {
    g_overlayVisible = !g_overlayVisible;
    g_needStaticRedraw = true;
    playUISound(SOUND_MODE_CHANGE);
  }
  if (zone == TOUCH_ZONE_ADVANCED) {
    g_advancedMode = !g_advancedMode;
    playUISound(SOUND_CONFIRM);
  }

  packetUpdate(now, &ds, g_estop);
  audioUpdate(now);

  // Static redraw on page/overlay change
  if (g_needStaticRedraw) {
    uiDrawCurrentPage();
    if (g_overlayVisible) uiDrawQuickActionOverlay();
    g_needStaticRedraw = false;
  }

  // Dynamic update
  TelemetryPacket telem;
  packetGetTelemetry(&telem);
  bool connected = packetTelemetryValid();

  TelemetryStripData strip = {};
  strip.batteryV = connected ? telem.batteryVoltage : 0.0f;
  strip.batteryPct = (strip.batteryV > 0) ? (int)((strip.batteryV - 3.0f) / 1.2f * 100.0f) : 0;
  if (strip.batteryPct < 0) strip.batteryPct = 0;
  if (strip.batteryPct > 100) strip.batteryPct = 100;
  strip.currentA = connected ? telem.currentDraw : 0.0f;
  strip.tempC = connected ? telem.temperature : 0.0f;
  strip.packetRate = espnowGetPacketRate();
  strip.rssi = 0;
  strip.connected = connected;
  strip.modeStr = g_estop ? "E-STOP" : (g_controlAuthority == CTRL_AUTONOMOUS ? "AUTO" :
                  g_controlAuthority == CTRL_SUPERVISED ? "SUPV" : "MANUAL");

  int lx, ly;
  touchGetJoystickDots(&lx, &ly);

  uiDrawUpdateDynamic(&strip, &ds, lx, ly);

  // Animated eye (Drive or Behaviour page)
  animUpdate(now);
  if (g_currentPage == PAGE_DRIVE || g_currentPage == PAGE_BEHAVIOUR) {
    animDrawEye(telem.moodState, g_estop, false);
  }

  if (g_advancedMode) {
    uiDrawAdvancedModeOverlay();
  }

  delay(5);
}
