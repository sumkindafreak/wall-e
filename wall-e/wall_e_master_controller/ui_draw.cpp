// ============================================================
//  WALL-E Master Controller — UI Draw Implementation
//  Zero-flicker, region-based, state-driven
// ============================================================

#include "ui_draw.h"
#include "animation_system.h"
#include "espnow_control.h"
#include <Arduino.h>
#include <stdio.h>
#include <math.h>

TFT_eSPI* g_tft = nullptr;

// ------------------------------------------------------------
//  Cached values (only redraw when changed)
// ------------------------------------------------------------
static float s_lastBatV = -1.0f;
static int s_lastBatPct = -1;
static float s_lastCurrent = -1.0f;
static float s_lastTemp = -1.0f;
static uint16_t s_lastPacketRate = 999;
static const char* s_lastModeStr = nullptr;
static int s_lastJoyDotX = JOY_CX, s_lastJoyDotY = JOY_CY;
static int s_lastLeftSpeed = 999, s_lastRightSpeed = 999;
static const int DOT_R = 8;  // Larger dot for single joystick

void uiDrawInit(TFT_eSPI* tft) {
  g_tft = tft;
}

// ------------------------------------------------------------
//  drawCurrentPage — central static draw
// ------------------------------------------------------------
void uiDrawCurrentPage(void) {
  if (!g_tft) return;

#if USE_PHYSICAL_JOYSTICKS
  if (g_inputMode == INPUT_PHYSICAL_JOYSTICK) {
    uiDrawPhysicalJoystickLayout();
    return;
  }
#endif

  switch (g_currentPage) {
    case PAGE_DRIVE:
      uiDrawStaticDrive();
      break;
    case PAGE_BEHAVIOUR:
      uiDrawStaticBehaviour();
      break;
    case PAGE_SYSTEM:
      uiDrawStaticSystem();
      break;
  }
}

// ------------------------------------------------------------
//  Static Drive (touchscreen, virtual joysticks)
// ------------------------------------------------------------
void uiDrawStaticDrive(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);

  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT - 1, SCREEN_W, C_BORDER);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("WALL-E", 10, 6);
  
  // Control authority indicator (drawn in top bar initially)
  uiDrawControlAuthority();

  g_tft->fillRect(0, TOP_BAR_HEIGHT, SCREEN_W, TELEM_STRIP_H, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT + TELEM_STRIP_H - 1, SCREEN_W, C_BORDER);
  
  // Force initial telemetry and control authority draw
  s_lastBatV = -999.0f;
  s_lastModeStr = nullptr;
  s_lastPacketRate = 9999;
  
  // Draw initial eye
  animDrawEye(0, false, true);

  // Draw 8-direction arrow buttons
  for (int i = 0; i < 8; i++) {
    float angle = i * 45.0f - 90.0f;  // 0=up
    float rad = angle * M_PI / 180.0f;
    int bx = JOY_CX + (int)(cosf(rad) * JOY_RADIUS);
    int by = JOY_CY + (int)(sinf(rad) * JOY_RADIUS);
    
    // Draw arrow button
    g_tft->fillCircle(bx, by, 18, C_BG_DARK);
    g_tft->drawCircle(bx, by, 18, C_ACCENT_DIM);
    
    // Draw arrow direction indicator
    int ax = bx + (int)(cosf(rad) * 8);
    int ay = by + (int)(sinf(rad) * 8);
    g_tft->fillCircle(ax, ay, 3, C_ACCENT);
  }
  
  // Draw center circle (dead zone indicator)
  g_tft->drawCircle(JOY_CX, JOY_CY, 30, C_GRID);
  g_tft->drawCircle(JOY_CX, JOY_CY, 2, C_ACCENT);

  int ex = SCREEN_W / 2 - 50;
  int ey = BOTTOM_BAR_Y + 4;
  int ew = 100, eh = 32;
  g_tft->fillRoundRect(ex, ey, ew, eh, 4, C_RED);
  g_tft->drawRoundRect(ex, ey, ew, eh, 4, C_WHITE);
  g_tft->setTextColor(C_WHITE, C_RED);
  g_tft->setTextSize(2);
  g_tft->drawString("E-STOP", ex + 22, ey + 8);

  g_tft->drawFastHLine(0, BOTTOM_BAR_Y, SCREEN_W, C_BORDER);
  g_tft->fillRect(0, BOTTOM_BAR_Y, SCREEN_W, BOTTOM_BAR_H, C_BG_DARK);
  g_tft->drawRect(SCREEN_W - 70, BOTTOM_BAR_Y + 8, 60, 24, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawString("Behav", SCREEN_W - 62, BOTTOM_BAR_Y + 14);
  g_tft->drawRect(SCREEN_W - 138, BOTTOM_BAR_Y + 8, 60, 24, C_BORDER);
  g_tft->drawString("System", SCREEN_W - 130, BOTTOM_BAR_Y + 14);
}

void uiDrawStaticBehaviour(void) {
  uiDrawPageBehaviour();
}

void uiDrawStaticSystem(void) {
  uiDrawPageSystem();
}

#if USE_PHYSICAL_JOYSTICKS
void uiDrawPhysicalJoystickLayout(void) {
  const int cTop = TOP_BAR_HEIGHT + TELEM_STRIP_H;
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->fillRect(0, TOP_BAR_HEIGHT, SCREEN_W, TELEM_STRIP_H, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT + TELEM_STRIP_H - 1, SCREEN_W, C_BORDER);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("WALL-E Console", 10, 6);

  int midX = SCREEN_W / 2;
  g_tft->drawFastVLine(midX, cTop, CONTENT_H, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->setTextSize(1);
  g_tft->drawString("Battery", 8, cTop + 4);
  g_tft->drawString("Behaviour", midX + 8, cTop + 4);

  const char* moods[] = {"Curious", "Happy", "Shy", "Tired", "Excited"};
  for (int i = 0; i < 5; i++) {
    int bx = midX + 16 + (i % 2) * 72;
    int by = cTop + 30 + (i / 2) * 45;
    g_tft->drawRect(bx, by, 64, 32, C_BORDER);
    g_tft->drawString(moods[i], bx + 4, by + 10);
  }
}
#endif

// ------------------------------------------------------------
//  Telemetry strip (battery bar, V, temp, current, pkt/s, mode)
// ------------------------------------------------------------
void uiDrawTelemetryStrip(const TelemetryStripData* telem) {
  if (!g_tft || !telem) return;

  int y = TOP_BAR_HEIGHT + 4;
  int w = 60, h = 8;
  int batPct = telem->batteryPct;
  if (batPct < 0) batPct = 0;
  if (batPct > 100) batPct = 100;

  bool changed = (telem->batteryV != s_lastBatV || batPct != s_lastBatPct ||
                  telem->currentA != s_lastCurrent || telem->tempC != s_lastTemp ||
                  telem->packetRate != s_lastPacketRate || telem->modeStr != s_lastModeStr);
  if (!changed) return;

  s_lastBatV = telem->batteryV;
  s_lastBatPct = batPct;
  s_lastCurrent = telem->currentA;
  s_lastTemp = telem->tempC;
  s_lastPacketRate = telem->packetRate;
  s_lastModeStr = telem->modeStr;

  g_tft->fillRect(0, TOP_BAR_HEIGHT, SCREEN_W, TELEM_STRIP_H, C_BG_DARK);
  g_tft->drawRect(10, y, w + 2, h + 2, C_BORDER);
  g_tft->fillRect(11, y + 1, (w * batPct) / 100, h, telem->connected ? C_GREEN : C_ACCENT_DIM);
  g_tft->fillRect(11 + (w * batPct) / 100, y + 1, w - (w * batPct) / 100, h, C_BG);

  char buf[80];
  snprintf(buf, sizeof(buf), "%.2fV | %.1fC | %.2fA | %up/s | %s",
           telem->batteryV, telem->tempC, telem->currentA,
           (unsigned)telem->packetRate, telem->modeStr ? telem->modeStr : "--");
  g_tft->setTextColor(telem->connected ? C_WHITE : C_TEXT_DIM, C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawString(buf, 78, y);
}

// ------------------------------------------------------------
//  Control authority indicator (with pulse for AUTO/SUPV/WARNING)
// ------------------------------------------------------------
static uint16_t lerpRGB(uint16_t a, uint16_t b, float t) {
  int r0 = (a >> 11) & 0x1F, g0 = (a >> 5) & 0x3F, b0 = a & 0x1F;
  int r1 = (b >> 11) & 0x1F, g1 = (b >> 5) & 0x3F, b1 = b & 0x1F;
  int r = (int)(r0 + (r1 - r0) * t) & 0x1F;
  int g = (int)(g0 + (g1 - g0) * t) & 0x3F;
  int bv = (int)(b0 + (b1 - b0) * t) & 0x1F;
  return (r << 11) | (g << 5) | bv;
}

void uiDrawControlAuthority(void) {
  if (!g_tft) return;
  int x = SCREEN_W - 120;
  int y = 4;
  uint16_t color = C_GREEN;
  const char* label = "LOCAL";
  bool pulse = false;
  uint16_t pulseColor = C_BLUE;
  switch (g_controlAuthority) {
    case CTRL_AUTONOMOUS: color = C_BLUE;  label = "AUTO"; pulse = true; pulseColor = C_BLUE;  break;
    case CTRL_SUPERVISED: color = C_YELLOW; label = "SUPV"; pulse = true; pulseColor = C_YELLOW; break;
    case CTRL_SAFETY:     color = C_RED;    label = "SAFE"; pulse = true; pulseColor = C_RED; break;
    default: break;
  }
  float brightness = animGetPulseBrightness();
  uint16_t bg = pulse ? lerpRGB(C_BG_DARK, pulseColor, brightness * 0.25f) : C_BG_DARK;
  g_tft->fillRect(x, y, 110, 18, bg);
  g_tft->setTextColor(color, bg);
  g_tft->setTextSize(1);
  g_tft->drawString("CTRL:", x, y);
  g_tft->drawString(label, x + 30, y);
}

// ------------------------------------------------------------
//  Update dynamic regions (button highlight, telemetry, eye)
// ------------------------------------------------------------
void uiDrawUpdateDynamic(const TelemetryStripData* telem, const DriveState* ds,
                         int joyDotX, int joyDotY) {
  if (!g_tft) return;

  // Drive page: highlight active button
  if (g_inputMode == INPUT_TOUCHSCREEN && g_currentPage == PAGE_DRIVE && !g_overlayVisible) {
    bool dotChanged = (joyDotX != s_lastJoyDotX || joyDotY != s_lastJoyDotY);
    if (dotChanged) {
      // Redraw old button (un-highlight)
      if (s_lastJoyDotX != JOY_CX || s_lastJoyDotY != JOY_CY) {
        g_tft->fillCircle(s_lastJoyDotX, s_lastJoyDotY, 18, C_BG_DARK);
        g_tft->drawCircle(s_lastJoyDotX, s_lastJoyDotY, 18, C_ACCENT_DIM);
        // Redraw arrow
        float angle = atan2f(s_lastJoyDotY - JOY_CY, s_lastJoyDotX - JOY_CX);
        int ax = s_lastJoyDotX + (int)(cosf(angle) * 8);
        int ay = s_lastJoyDotY + (int)(sinf(angle) * 8);
        g_tft->fillCircle(ax, ay, 3, C_ACCENT);
      }
      
      // Highlight new button
      if (joyDotX != JOY_CX || joyDotY != JOY_CY) {
        g_tft->fillCircle(joyDotX, joyDotY, 18, C_ACCENT);
        g_tft->drawCircle(joyDotX, joyDotY, 18, C_WHITE);
      }
      
      s_lastJoyDotX = joyDotX;
      s_lastJoyDotY = joyDotY;
    }
  }

  if (telem) uiDrawTelemetryStrip(telem);
  uiDrawControlAuthority();
}

void uiDrawEStopRegion(bool highlighted) {
  if (!g_tft) return;
  int ex = SCREEN_W / 2 - 50, ey = BOTTOM_BAR_Y + 4, ew = 100, eh = 32;
  g_tft->fillRoundRect(ex, ey, ew, eh, 4, highlighted ? 0xFF00 : C_RED);
  g_tft->drawRoundRect(ex, ey, ew, eh, 4, C_WHITE);
  g_tft->setTextColor(C_WHITE, highlighted ? 0xFF00 : C_RED);
  g_tft->setTextSize(2);
  g_tft->drawString("E-STOP", ex + 22, ey + 8);
}

void uiDrawPageBehaviour(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Behaviour", 10, 6);
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->setTextSize(1);
  const char* moods[] = {"Curious", "Happy", "Shy", "Tired", "Excited"};
  for (int i = 0; i < 5; i++) {
    int bx = 16 + (i % 2) * 150;
    int by = 50 + (i / 2) * 50;
    g_tft->drawRect(bx, by, 120, 36, C_BORDER);
    g_tft->drawString(moods[i], bx + 8, by + 12);
  }
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

void uiDrawPageSystem(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("System", 10, 6);
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  g_tft->drawString("Battery: -- V", 16, 56);
  g_tft->drawString("Current: -- A", 16, 80);
  g_tft->drawString("Temp: -- C", 16, 104);
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

// ------------------------------------------------------------
//  Quick action overlay (long-press bottom-right)
// ------------------------------------------------------------
void uiDrawQuickActionOverlay(void) {
  if (!g_tft) return;
  int ox = 40, oy = 60, ow = 240, oh = 140;
  g_tft->fillRect(ox, oy, ow, oh, C_BG_DARK);
  g_tft->drawRect(ox, oy, ow, oh, C_ACCENT);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Quick Actions", ox + 60, oy + 8);

  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawRect(ox + 10, oy + 32, ow - 20, 24, C_BORDER);
  g_tft->drawString("Calibrate IMU", ox + 18, oy + 38);
  g_tft->drawRect(ox + 10, oy + 60, ow - 20, 24, C_BORDER);
  g_tft->drawString("Reset Motors", ox + 18, oy + 66);
  g_tft->drawRect(ox + 10, oy + 88, ow - 20, 24, C_BORDER);
  g_tft->drawString("Supervised Mode", ox + 18, oy + 94);
  g_tft->drawRect(ox + 10, oy + 116, ow - 20, 24, C_BORDER);
  g_tft->drawString("Reboot Base", ox + 18, oy + 122);
}

void uiDrawAdvancedModeOverlay(void) {
  if (!g_tft || !g_advancedMode) return;
  int x = 4, y = TOP_BAR_HEIGHT + TELEM_STRIP_H + 4;
  g_tft->fillRect(x, y, 120, 52, C_BG_DARK);
  g_tft->drawRect(x, y, 120, 52, C_ACCENT_DIM);
  g_tft->setTextColor(C_YELLOW, C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawString("ADV MODE", x + 4, y + 2);
  g_tft->setTextColor(C_TEXT_DIM, C_BG_DARK);
  g_tft->drawString("Motor L: --% R: --%", x + 4, y + 14);
  g_tft->drawString("IMU: --", x + 4, y + 26);
  g_tft->drawString("CPU: --% Lat: --ms", x + 4, y + 38);
}
