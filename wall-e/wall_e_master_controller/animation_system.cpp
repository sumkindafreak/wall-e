// ============================================================
//  WALL-E Master Controller — Animation System Implementation
//  Blink 4-8s random, mood glow, E-STOP flash, pulse effect
// ============================================================

#include "animation_system.h"
#include "ui_draw.h"
#include <Arduino.h>
#include <math.h>

#define C_ACCENT    0xFD20
#define C_ACCENT_DIM 0xB360
#define C_BG_DARK   0x18C3
#define C_RED       0xF800
#define C_YELLOW    0xFFE0
#define C_ORANGE_DIM 0xB360
#define BLINK_MIN_MS  4000
#define BLINK_MAX_MS  8000
#define BLINK_DURATION_MS  120

static unsigned long s_lastBlinkMs = 0;
static unsigned long s_blinkIntervalMs = 5000;  // Next blink
static bool s_eyeOpen = true;

void animInit(void) {
  s_lastBlinkMs = millis();
  s_blinkIntervalMs = BLINK_MIN_MS + (millis() % (BLINK_MAX_MS - BLINK_MIN_MS));
  s_eyeOpen = true;
}

void animUpdate(unsigned long now) {
  if (s_eyeOpen) {
    if (now - s_lastBlinkMs > s_blinkIntervalMs) {
      s_eyeOpen = false;
      s_lastBlinkMs = now;
      s_blinkIntervalMs = BLINK_MIN_MS + (now % (BLINK_MAX_MS - BLINK_MIN_MS));
    }
  } else {
    if (now - s_lastBlinkMs > BLINK_DURATION_MS) {
      s_eyeOpen = true;
      s_lastBlinkMs = now;
    }
  }
}

void animDrawEye(uint8_t moodState, bool estop, bool forceRedraw) {
  if (!g_tft) return;

  int x = EYE_REGION_X;
  int y = EYE_REGION_Y;
  int w = EYE_REGION_W;
  int h = EYE_REGION_H;

  int eyeW = 8;
  uint16_t glowColor = C_ACCENT_DIM;
  if (moodState == 1 || moodState == 4) { eyeW = 12; glowColor = C_YELLOW; }  // Happy/Excited
  if (moodState == 2 || moodState == 3) { eyeW = 5;  glowColor = C_ORANGE_DIM; }  // Shy/Tired
  if (estop) { eyeW = 16; glowColor = C_RED; }  // E-STOP flash wide
  if (!s_eyeOpen) eyeW = 0;

  // Region-only redraw
  g_tft->fillRect(x, y, w, h, glowColor);  // Mood glow background
  g_tft->drawRect(x, y, w, h, C_ACCENT_DIM);

  if (s_eyeOpen && eyeW > 0) {
    int cx = x + w / 2;
    int cy = y + h / 2;
    int hw = eyeW / 2;
    g_tft->fillRoundRect(cx - hw, cy - 4, eyeW, 8, 4, C_ACCENT);
  }
}

float animGetPulseBrightness(void) {
  float t = (float)millis() * 0.005f;
  return (sinf(t) * 0.5f + 0.5f);
}
