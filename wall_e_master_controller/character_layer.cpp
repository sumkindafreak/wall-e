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

  // Status indicator dot (top-right corner)
  // Clear area and draw blinking dot
  g_tft->fillRect(x, y, EYELET_W, EYELET_H, C_BG_DARK);
  
  // Determine dot color based on mood/state
  uint16_t dotColor = C_ACCENT;
  if (estop) dotColor = C_RED;  // Red when E-STOP
  
  // Blink: only draw when eye is "open"
  if (s_eyeOpen) {
    int cx = x + EYELET_W / 2;
    int cy = y + EYELET_H / 2;
    g_tft->fillCircle(cx, cy, 6, dotColor);  // 6px radius dot
    g_tft->drawCircle(cx, cy, 6, C_ACCENT_DIM);  // Dim outline
  }
}
