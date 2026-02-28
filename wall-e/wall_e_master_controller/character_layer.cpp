// ============================================================
//  WALL-E Master Controller — Character Layer Implementation
//  Eye: blink every few seconds, mood-based width, E-STOP flash
// ============================================================

#include "character_layer.h"
#include "ui_draw.h"
#include <Arduino.h>

#define C_ACCENT    0xFD20
#define C_ACCENT_DIM 0xB360
#define C_BG_DARK   0x18C3
#define C_RED       0xF800
#define BLINK_INTERVAL_MS  4000
#define BLINK_DURATION_MS  150

static unsigned long s_lastBlinkMs = 0;
static bool s_eyeOpen = true;

void charLayerInit(void) {
  s_lastBlinkMs = millis();
  s_eyeOpen = true;
}

void charLayerUpdate(unsigned long now) {
  if (s_eyeOpen && (now - s_lastBlinkMs > BLINK_INTERVAL_MS)) {
    s_eyeOpen = false;
    s_lastBlinkMs = now;
  } else if (!s_eyeOpen && (now - s_lastBlinkMs > BLINK_DURATION_MS)) {
    s_eyeOpen = true;
    s_lastBlinkMs = now;
  }
}

void charLayerDraw(uint8_t moodState, bool estop) {
  if (!g_tft) return;

  int x = EYELET_X;
  int y = EYELET_Y;
  int w = EYELET_W;
  int h = EYELET_H;

  // Mood affects eye width: 0=normal, 1=wide, 2=narrow, 3=narrow, 4=wide
  int eyeW = 8;
  if (moodState == 1 || moodState == 4) eyeW = 12;
  if (moodState == 2 || moodState == 3) eyeW = 5;
  if (estop) eyeW = 14;  // E-STOP: flash wide
  if (!s_eyeOpen) eyeW = 0;

  g_tft->fillRect(x, y, w, h, C_BG_DARK);
  g_tft->drawRect(x, y, w, h, C_ACCENT_DIM);

  if (s_eyeOpen && eyeW > 0) {
    int cx = x + w / 2;
    int cy = y + h / 2;
    int hw = eyeW / 2;
    g_tft->fillRoundRect(cx - hw, cy - 4, eyeW, 8, 4, C_ACCENT);
  }
}
