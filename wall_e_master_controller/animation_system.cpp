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

  // Determine dot color based on mood/state
  uint16_t dotColor = C_ACCENT;
  if (estop) dotColor = C_RED;  // Red when E-STOP
  else if (moodState == 1 || moodState == 4) dotColor = C_YELLOW;  // Happy/Excited
  else if (moodState == 2 || moodState == 3) dotColor = C_ORANGE_DIM;  // Shy/Tired

  // Calculate dot center position
  int cx = x + w / 2;
  int cy = y + h / 2;
  
  // Only clear the dot area (small circle), not the whole region
  static bool wasOpen = true;
  
  if (s_eyeOpen != wasOpen) {
    // State changed - need to redraw
    if (s_eyeOpen) {
      // Draw dot
      g_tft->fillCircle(cx, cy, 6, dotColor);
      g_tft->drawCircle(cx, cy, 6, C_ACCENT_DIM);
    } else {
      // Erase dot (draw black circle)
      g_tft->fillCircle(cx, cy, 7, C_BG_DARK);
    }
    wasOpen = s_eyeOpen;
  } else if (s_eyeOpen) {
    // Just keep dot visible (in case something overwrote it)
    g_tft->fillCircle(cx, cy, 6, dotColor);
    g_tft->drawCircle(cx, cy, 6, C_ACCENT_DIM);
  }
}

float animGetPulseBrightness(void) {
  float t = (float)millis() * 0.005f;
  return (sinf(t) * 0.5f + 0.5f);
}
