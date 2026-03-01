// ============================================================
//  WALL-E Master Controller — UI Draw Implementation
//  Zero-flicker, region-based, state-driven
// ============================================================

#include "ui_draw.h"
#include "animation_system.h"
#include "espnow_control.h"
#include "profiles.h"
#include "motion_engine.h"  // For SERVO_COUNT and motionGetServoTargets
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

// ============================================================
//  Helper: Get Autonomy State Name
// ============================================================
static const char* getAutonomyStateName(uint8_t state) {
  switch (state) {
    case 0: return "IDLE";
    case 1: return "SCAN";
    case 2: return "EVALUATE";
    case 3: return "APPROACH";
    case 4: return "INVESTIGATE";
    case 5: return "REACT";
    case 6: return "WANDER";
    case 7: return "AVOID";
    case 8: return "ORIENT";
    case 9: return "EXPLORE";
    case 10: return "WAYPOINT_NAV";
    default: return "UNKNOWN";
  }
}

// ------------------------------------------------------------
//  drawCurrentPage — central static draw
// ------------------------------------------------------------
void uiDrawCurrentPage(void) {
  if (!g_tft) return;

#if USE_PHYSICAL_JOYSTICKS
  // Physical joystick mode: only use special layout on PAGE_DRIVE
  if (g_inputMode == INPUT_PHYSICAL_JOYSTICK && g_currentPage == PAGE_DRIVE) {
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
    case PAGE_PROFILE:
      uiDrawStaticProfile();
      break;
    case PAGE_SERVO_EDITOR:
      uiDrawStaticServoEditor();
      break;
    case PAGE_SERVO_TEST:
      uiDrawStaticServoTest();
      break;
    case PAGE_AUTONOMY:
      uiDrawPageAutonomy();
      break;
    case PAGE_WAYPOINTS:
      uiDrawPageAutonomy();  // Placeholder - same as autonomy for now
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
  
  // Get animation names based on current profile's favorites
  Profile* p = profileGet();
  const char* allAnimNames[] = {"Reset", "Bootup", "Inquis", "BrowR", "BrowL", "Suprs"};
  const char* displayNames[5];
  
  // Use favorite animations from profile
  for (int i = 0; i < 5; i++) {
    uint8_t animId = p->favoriteAnimations[i];
    if (animId < 6) {
      displayNames[i] = allAnimNames[animId];
    } else {
      displayNames[i] = "---";  // Empty slot
    }
  }
  
  for (int i = 0; i < 5; i++) {
    int bx = midX + 16 + (i % 2) * 72;
    int by = cTop + 30 + (i / 2) * 45;
    g_tft->drawRect(bx, by, 64, 32, C_BORDER);
    g_tft->drawString(displayNames[i], bx + 4, by + 10);
  }
  
  // NAVIGATION BUTTONS (left side under "Battery")
  // System button
  g_tft->fillRect(8, cTop + 30, 80, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->setTextSize(1);
  g_tft->drawString("System", 22, cTop + 40);
  
  // Behaviour button
  g_tft->drawRect(8, cTop + 68, 80, 32, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->drawString("Behav", 25, cTop + 78);
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

  // Draw battery percentage text inside/next to bar
  char batText[8];
  snprintf(batText, sizeof(batText), "%d%%", batPct);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawString(batText, 14, y + 1);  // Inside the battery bar

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
  
  // Only fill the exact text area, not a big rectangle
  // "CTRL:" = ~30px, label = ~30px, total ~65px width
  g_tft->fillRect(x, y, 65, 10, bg);
  
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
  g_tft->drawString("Animations", 10, 6);
  
  // Display all available animations (6 total)
  // Animation names from animation_data.h
  const char* animNames[] = {"Reset", "Bootup", "Inquis", "BrowR", "BrowL", "Suprs"};
  
  // Get current profile's favorites
  Profile* p = profileGet();
  
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->setTextSize(1);
  
  // Draw 6 animation buttons (3x2 grid)
  for (int i = 0; i < 6; i++) {
    int bx = 16 + (i % 3) * 100;
    int by = 50 + (i / 3) * 50;
    
    // Check if this animation is favorited
    bool isFavorite = false;
    for (int f = 0; f < 5; f++) {
      if (p->favoriteAnimations[f] == i) {
        isFavorite = true;
        break;
      }
    }
    
    // Draw button border (highlight if favorite)
    uint16_t borderColor = isFavorite ? TFT_YELLOW : C_BORDER;
    g_tft->drawRect(bx, by, 90, 36, borderColor);
    if (isFavorite) {
      g_tft->drawRect(bx + 1, by + 1, 88, 34, borderColor);  // Double border for favorites
    }
    
    g_tft->setTextColor(C_ACCENT, C_BG);
    g_tft->drawString(animNames[i], bx + 8, by + 12);
    
    // Show animation ID
    char idBuf[4];
    snprintf(idBuf, sizeof(idBuf), "%d", i);
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->drawString(idBuf, bx + 75, by + 12);
    
    // Draw star for favorites
    if (isFavorite) {
      g_tft->setTextColor(TFT_YELLOW, C_BG);
      g_tft->drawString("*", bx + 75, by + 22);
    }
    
    g_tft->setTextColor(C_ACCENT, C_BG);
  }
  
  // Instructions
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  g_tft->drawString("Tap to play | Hold to fav", 10, BOTTOM_BAR_Y - 15);
  
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
  
  // Profiles button
  g_tft->fillRect(16, 130, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Profiles", 32, 140);
  
  // Servo Test button
  g_tft->fillRect(130, 130, 100, 32, C_ACCENT);
  g_tft->drawString("Servos", 150, 140);
  
  // NEW: Autonomy button
  g_tft->fillRect(16, 168, 100, 32, C_GREEN);
  g_tft->drawString("Autonomy", 28, 178);
  
  // Back button
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

// ============================================================
//  Autonomy Page
// ============================================================
void uiDrawPageAutonomy(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  
  // Grid
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);
  
  // Top bar
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Autonomy", 10, 6);
  
  // Get telemetry
  TelemetryPacket telem;
  packetGetTelemetry(&telem);
  
  // Enable/Disable toggle button
  bool enabled = telem.autonomyEnabled;
  uint16_t btnColor = enabled ? C_GREEN : C_RED;
  g_tft->fillRect(200, 50, 100, 32, btnColor);
  g_tft->setTextColor(C_WHITE, btnColor);
  g_tft->setTextSize(1);
  g_tft->drawString(enabled ? "ENABLED" : "DISABLED", 215, 60);
  
  // State
  g_tft->setTextColor(C_ACCENT, C_BG);
  char buf[64];
  snprintf(buf, sizeof(buf), "State: %s", getAutonomyStateName(telem.autonomyState));
  g_tft->drawString(buf, 16, 56);
  
  // Sonar
  snprintf(buf, sizeof(buf), "Sonar: %.1fcm", telem.sonarDistanceCm);
  g_tft->drawString(buf, 16, 80);
  
  // Compass
  snprintf(buf, sizeof(buf), "Heading: %.0f°", telem.compassHeading);
  g_tft->drawString(buf, 16, 104);
  
  // GPS
  if (telem.gpsValid) {
    snprintf(buf, sizeof(buf), "GPS: %.5f,%.5f", telem.gpsLatitude, telem.gpsLongitude);
  } else {
    snprintf(buf, sizeof(buf), "GPS: No fix");
  }
  g_tft->drawString(buf, 16, 128);
  
  // Waypoint info
  if (telem.waypointMode) {
    snprintf(buf, sizeof(buf), "WP: %d/%d", telem.currentWaypoint + 1, telem.totalWaypoints);
    g_tft->drawString(buf, 16, 152);
    snprintf(buf, sizeof(buf), "Dist: %.1fm @ %.0f°", telem.waypointDistanceM, telem.waypointBearingDeg);
    g_tft->drawString(buf, 16, 176);
  }
  
  // Back button
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

// ------------------------------------------------------------
//  Profile Selection Page
// ------------------------------------------------------------
void uiDrawStaticProfile(void) {
  if (!g_tft) return;
  
  g_tft->fillScreen(C_BG);
  
  // Top bar
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT - 1, SCREEN_W, C_BORDER);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Profile", 10, 6);
  
  // Profile cards (3 profiles)
  const int cardW = 90;
  const int cardH = 120;
  const int cardSpacing = 10;
  const int startX = (SCREEN_W - (cardW * 3 + cardSpacing * 2)) / 2;
  const int startY = 50;
  
  // Get current profile
  Profile* currentProfile = profileGet();
  
  // Draw 3 profile cards
  for (int i = 0; i < PROFILE_COUNT; i++) {
    int x = startX + i * (cardW + cardSpacing);
    int y = startY;
    
    bool isActive = (i == g_currentProfile);
    uint16_t bgColor = isActive ? C_ACCENT : C_BG_DARK;
    uint16_t borderColor = isActive ? C_WHITE : C_BORDER;
    uint16_t textColor = isActive ? C_BG : C_WHITE;
    
    // Card background
    g_tft->fillRect(x, y, cardW, cardH, bgColor);
    g_tft->drawRect(x, y, cardW, cardH, borderColor);
    
    // Profile icon/number
    g_tft->setTextSize(3);
    g_tft->setTextColor(textColor, bgColor);
    char numStr[3];
    snprintf(numStr, sizeof(numStr), "%d", i);
    g_tft->drawString(numStr, x + cardW / 2 - 8, y + 10);
    
    // Profile name
    g_tft->setTextSize(1);
    g_tft->drawString(profiles[i].name, x + (cardW - strlen(profiles[i].name) * 6) / 2, y + 45);
    
    // Profile details
    g_tft->setTextSize(1);
    char detailBuf[20];
    
    // Speed
    snprintf(detailBuf, sizeof(detailBuf), "Spd:%d%%", (int)(profiles[i].joystickMaxSpeed * 100));
    g_tft->drawString(detailBuf, x + 5, y + 65);
    
    // Deadzone
    snprintf(detailBuf, sizeof(detailBuf), "DZ:%d%%", (int)(profiles[i].joystickDeadzone * 100));
    g_tft->drawString(detailBuf, x + 5, y + 80);
    
    // Expo
    snprintf(detailBuf, sizeof(detailBuf), "Exp:%d%%", (int)(profiles[i].joystickExpo * 100));
    g_tft->drawString(detailBuf, x + 5, y + 95);
    
    // Edit button at bottom of card
    g_tft->drawRect(x + 5, y + cardH - 24, cardW - 10, 20, C_BORDER);
    g_tft->setTextColor(C_ACCENT, bgColor);
    g_tft->drawString("Edit", x + cardW / 2 - 12, y + cardH - 19);
  }
  
  // Back button
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->setTextSize(1);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

// ------------------------------------------------------------
//  Servo Editor Page
// ------------------------------------------------------------
void uiDrawStaticServoEditor(void) {
  if (!g_tft) return;
  
  g_tft->fillScreen(C_BG);
  
  // Top bar with profile name
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT - 1, SCREEN_W, C_BORDER);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  
  Profile* p = profileGet();
  char titleBuf[32];
  snprintf(titleBuf, sizeof(titleBuf), "%s Tuning", p->name);
  g_tft->drawString(titleBuf, 10, 6);
  
  // Adjustment sliders
  const int startY = 45;
  const int sliderH = 24;
  const int spacing = 28;
  const int labelX = 10;
  const int sliderX = 120;
  const int sliderW = 180;
  
  g_tft->setTextSize(1);
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  
  // 1. Head Sensitivity
  g_tft->drawString("Head Sens:", labelX, startY);
  g_tft->drawRect(sliderX, startY - 2, sliderW, sliderH, C_BORDER);
  int headW = (int)(p->headSensitivity / 2.0f * sliderW);  // 0.5-2.0 → 0-100%
  g_tft->fillRect(sliderX + 2, startY, headW, sliderH - 4, C_ACCENT);
  char valBuf[8];
  snprintf(valBuf, sizeof(valBuf), "%.1fx", p->headSensitivity);
  g_tft->setTextColor(C_WHITE, C_BG);
  g_tft->drawString(valBuf, sliderX + sliderW + 5, startY + 6);
  
  // 2. Servo Speed
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->drawString("Servo Speed:", labelX, startY + spacing);
  g_tft->drawRect(sliderX, startY + spacing - 2, sliderW, sliderH, C_BORDER);
  int speedW = (int)(p->servoSpeedLimit * sliderW);
  g_tft->fillRect(sliderX + 2, startY + spacing, speedW, sliderH - 4, C_ACCENT);
  snprintf(valBuf, sizeof(valBuf), "%d%%", (int)(p->servoSpeedLimit * 100));
  g_tft->setTextColor(C_WHITE, C_BG);
  g_tft->drawString(valBuf, sliderX + sliderW + 5, startY + spacing + 6);
  
  // 3. Joy Deadzone
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->drawString("Deadzone:", labelX, startY + spacing * 2);
  g_tft->drawRect(sliderX, startY + spacing * 2 - 2, sliderW, sliderH, C_BORDER);
  int dzW = (int)(p->joystickDeadzone * 2.0f * sliderW);  // 0-0.5 range
  g_tft->fillRect(sliderX + 2, startY + spacing * 2, dzW, sliderH - 4, C_ACCENT);
  snprintf(valBuf, sizeof(valBuf), "%d%%", (int)(p->joystickDeadzone * 100));
  g_tft->setTextColor(C_WHITE, C_BG);
  g_tft->drawString(valBuf, sliderX + sliderW + 5, startY + spacing * 2 + 6);
  
  // 4. Joy Expo
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->drawString("Expo Curve:", labelX, startY + spacing * 3);
  g_tft->drawRect(sliderX, startY + spacing * 3 - 2, sliderW, sliderH, C_BORDER);
  int expoW = (int)(p->joystickExpo * sliderW);
  g_tft->fillRect(sliderX + 2, startY + spacing * 3, expoW, sliderH - 4, C_ACCENT);
  snprintf(valBuf, sizeof(valBuf), "%d%%", (int)(p->joystickExpo * 100));
  g_tft->setTextColor(C_WHITE, C_BG);
  g_tft->drawString(valBuf, sliderX + sliderW + 5, startY + spacing * 3 + 6);
  
  // 5. Max Speed
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->drawString("Max Speed:", labelX, startY + spacing * 4);
  g_tft->drawRect(sliderX, startY + spacing * 4 - 2, sliderW, sliderH, C_BORDER);
  int maxSpeedW = (int)(p->joystickMaxSpeed * sliderW);
  g_tft->fillRect(sliderX + 2, startY + spacing * 4, maxSpeedW, sliderH - 4, C_ACCENT);
  snprintf(valBuf, sizeof(valBuf), "%d%%", (int)(p->joystickMaxSpeed * 100));
  g_tft->setTextColor(C_WHITE, C_BG);
  g_tft->drawString(valBuf, sliderX + sliderW + 5, startY + spacing * 4 + 6);
  
  // Action buttons at bottom
  // Save button (left)
  g_tft->fillRect(20, BOTTOM_BAR_Y + 4, 70, 32, C_GREEN);
  g_tft->setTextColor(C_BG, C_GREEN);
  g_tft->drawString("Save", 36, BOTTOM_BAR_Y + 14);
  
  // Reset button (center)
  g_tft->drawRect(110, BOTTOM_BAR_Y + 4, 70, 32, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->drawString("Reset", 122, BOTTOM_BAR_Y + 14);
  
  // Back button (right)
  g_tft->fillRect(SCREEN_W - 90, BOTTOM_BAR_Y + 4, 70, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W - 74, BOTTOM_BAR_Y + 14);
}

// ------------------------------------------------------------
//  Servo Test Page - Individual servo control (0-180°)
// ------------------------------------------------------------
void uiDrawStaticServoTest(void) {
  if (!g_tft) return;
  
  g_tft->fillScreen(C_BG);
  
  // Top bar
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT - 1, SCREEN_W, C_BORDER);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(2);
  g_tft->drawString("Servo Test", 10, 6);
  
  // Get current servo positions
  uint8_t servoTargets[SERVO_COUNT];
  motionGetServoTargets(servoTargets);
  
  // Servo names
  const char* servoNames[SERVO_COUNT] = {
    "Pan", "Tilt", "EyeL", "EyeR", "NeckT",
    "NeckB", "ArmL", "ArmR", "BrowR", "BrowL"
  };
  
  // Draw servo sliders (2 columns)
  const int startY = 40;
  const int sliderH = 16;
  const int spacing = 18;
  const int col1X = 10;
  const int col2X = 165;
  const int labelW = 40;
  const int sliderX = 55;
  const int sliderW = 90;
  
  g_tft->setTextSize(1);
  
  for (int i = 0; i < SERVO_COUNT; i++) {
    int colX = (i < 5) ? col1X : col2X;
    int row = (i < 5) ? i : (i - 5);
    int y = startY + (row * spacing);
    
    // Label
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->drawString(servoNames[i], colX, y + 3);
    
    // Slider background
    g_tft->drawRect(colX + sliderX, y, sliderW, sliderH, C_BORDER);
    
    // Slider fill (based on current position 0-180°)
    int fillW = (int)((float)servoTargets[i] / 180.0f * (sliderW - 4));
    g_tft->fillRect(colX + sliderX + 2, y + 2, fillW, sliderH - 4, C_ACCENT);
    
    // Value display
    char valBuf[6];
    snprintf(valBuf, sizeof(valBuf), "%d", servoTargets[i]);
    g_tft->setTextColor(C_WHITE, C_BG);
    g_tft->drawString(valBuf, colX + sliderX + sliderW + 5, y + 3);
  }
  
  // Preset buttons at bottom
  // Save Neutral button (left) - NEW
  g_tft->fillRect(5, BOTTOM_BAR_Y + 4, 70, 32, C_GREEN);
  g_tft->setTextColor(C_BG, C_GREEN);
  g_tft->drawString("SaveNeu", 12, BOTTOM_BAR_Y + 14);
  
  // Neutral button
  g_tft->drawRect(80, BOTTOM_BAR_Y + 4, 55, 32, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->drawString("Neut", 90, BOTTOM_BAR_Y + 14);
  
  // Test1 button
  g_tft->drawRect(140, BOTTOM_BAR_Y + 4, 45, 32, C_BORDER);
  g_tft->drawString("Test1", 146, BOTTOM_BAR_Y + 14);
  
  // Test2 button
  g_tft->drawRect(190, BOTTOM_BAR_Y + 4, 45, 32, C_BORDER);
  g_tft->drawString("Test2", 196, BOTTOM_BAR_Y + 14);
  
  // Back button (right)
  g_tft->fillRect(SCREEN_W - 70, BOTTOM_BAR_Y + 4, 60, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W - 54, BOTTOM_BAR_Y + 14);
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

// ------------------------------------------------------------
//  Physical Joystick Visual Indicators
//  Draw two mini joystick displays at bottom of screen
// ------------------------------------------------------------
void uiDrawPhysicalJoystickIndicators(float joy1X, float joy1Y, float joy2X, float joy2Y) {
  if (!g_tft) return;
  
  // Joy1 (Head control) - Left side
  const int joy1_cx = 60;
  const int joy1_cy = 210;
  const int joy_radius = 25;
  const int stick_radius = 6;
  
  // Joy2 (Tank drive) - Right side - shifted right 20px total
  const int joy2_cx = 280;  // Was 260, now 280 (+20px)
  const int joy2_cy = 210;
  
  // Clear previous indicators
  static int lastJoy1X = 0, lastJoy1Y = 0;
  static int lastJoy2X = 0, lastJoy2Y = 0;
  
  // Draw Joy1 base (left - head control)
  g_tft->drawCircle(joy1_cx, joy1_cy, joy_radius, C_ACCENT_DIM);
  g_tft->drawCircle(joy1_cx, joy1_cy, 2, C_ACCENT_DIM); // Center dot
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  g_tft->drawString("HEAD", joy1_cx - 15, joy1_cy - 40);
  
  // Draw Joy2 base (right - tank drive)
  g_tft->drawCircle(joy2_cx, joy2_cy, joy_radius, C_ACCENT);
  g_tft->drawCircle(joy2_cx, joy2_cy, 2, C_ACCENT); // Center dot
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->drawString("DRIVE", joy2_cx - 17, joy2_cy - 40);  // Back to left (was -7, now -17)
  
  // Erase old Joy1 stick position
  if (lastJoy1X != 0 || lastJoy1Y != 0) {
    g_tft->fillCircle(lastJoy1X, lastJoy1Y, stick_radius, C_BG);
  }
  
  // Erase old Joy2 stick position
  if (lastJoy2X != 0 || lastJoy2Y != 0) {
    g_tft->fillCircle(lastJoy2X, lastJoy2Y, stick_radius, C_BG);
  }
  
  // Calculate new positions (invert X to match physical joystick rotation)
  int joy1_x = joy1_cx - (int)(joy1X * joy_radius);  // Inverted X
  int joy1_y = joy1_cy + (int)(joy1Y * joy_radius);
  
  int joy2_x = joy2_cx - (int)(joy2X * joy_radius);  // Inverted X
  int joy2_y = joy2_cy + (int)(joy2Y * joy_radius);
  
  // Constrain to circle (with inverted X)
  float dist1 = sqrtf(joy1X * joy1X + joy1Y * joy1Y);
  if (dist1 > 1.0f) {
    joy1_x = joy1_cx - (int)((joy1X / dist1) * (joy_radius - stick_radius));  // Inverted X
    joy1_y = joy1_cy + (int)((joy1Y / dist1) * (joy_radius - stick_radius));
  }
  
  float dist2 = sqrtf(joy2X * joy2X + joy2Y * joy2Y);
  if (dist2 > 1.0f) {
    joy2_x = joy2_cx - (int)((joy2X / dist2) * (joy_radius - stick_radius));  // Inverted X
    joy2_y = joy2_cy + (int)((joy2Y / dist2) * (joy_radius - stick_radius));
  }
  
  // Draw new stick positions
  // Joy1 (dim - future head control)
  g_tft->fillCircle(joy1_x, joy1_y, stick_radius, C_ACCENT_DIM);
  g_tft->drawCircle(joy1_x, joy1_y, stick_radius, C_TEXT_DIM);
  
  // Joy2 (bright - active tank drive)
  bool joy2Active = (fabsf(joy2X) > 0.05f || fabsf(joy2Y) > 0.05f);
  uint16_t joy2Color = joy2Active ? C_ACCENT : C_ACCENT_DIM;
  g_tft->fillCircle(joy2_x, joy2_y, stick_radius, joy2Color);
  g_tft->drawCircle(joy2_x, joy2_y, stick_radius, C_WHITE);
  
  // Draw line from center to stick (Joy2 only when active)
  if (joy2Active) {
    g_tft->drawLine(joy2_cx, joy2_cy, joy2_x, joy2_y, C_ACCENT_DIM);
  }
  
  // Save positions for next erase
  lastJoy1X = joy1_x;
  lastJoy1Y = joy1_y;
  lastJoy2X = joy2_x;
  lastJoy2Y = joy2_y;
}
