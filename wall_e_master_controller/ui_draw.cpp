// ============================================================
//  WALL-E Master Controller — UI Draw Implementation
//  Zero-flicker, region-based, state-driven
// ============================================================

#include "ui_draw.h"
#include "ui_sd_explorer.h"
#include "ui_draw_laser.h"
#include "animation_system.h"
#include "espnow_control.h"
#include "packet_control.h"
#include "profiles.h"
#include "animation_data.h"
#include "motion_engine.h"  // For SERVO_COUNT and motionGetServoTargets
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
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
static char s_lastEmoStr[16] = "";
static int s_lastJoyDotX = JOY_CX, s_lastJoyDotY = JOY_CY;
static int s_lastLeftSpeed = 999, s_lastRightSpeed = 999;
static const int DOT_R = 8;  // Larger dot for single joystick

void uiDrawInit(TFT_eSPI* tft) {
  g_tft = tft;
}

int uiBannerTotalHeight(void) {
  return g_topBannerCollapsed ? BANNER_MINI_H : (TOP_BAR_HEIGHT + TELEM_STRIP_H);
}

int uiContentTop(void) {
  return uiBannerTotalHeight();
}

int uiContentHeight(void) {
  return BOTTOM_BAR_Y - uiContentTop();
}

void uiBannerInvalidateTelemetryCache(void) {
  s_lastBatV = -999.0f;
  s_lastBatPct = -1;
  s_lastCurrent = -999.0f;
  s_lastTemp = -999.0f;
  s_lastPacketRate = 9999;
  s_lastModeStr = nullptr;
  s_lastEmoStr[0] = '\0';
}

void uiDrawBannerBackground(void) {
  if (!g_tft) return;
  if (g_topBannerCollapsed) {
    g_tft->fillRect(0, 0, SCREEN_W, BANNER_MINI_H, C_BG_DARK);
    g_tft->drawFastHLine(0, BANNER_MINI_H - 1, SCREEN_W, C_BORDER);
    return;
  }
  g_tft->fillRect(0, 0, SCREEN_W, TOP_BAR_HEIGHT, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT - 1, SCREEN_W, C_BORDER);
  g_tft->fillRect(0, TOP_BAR_HEIGHT, SCREEN_W, TELEM_STRIP_H, C_BG_DARK);
  g_tft->drawFastHLine(0, TOP_BAR_HEIGHT + TELEM_STRIP_H - 1, SCREEN_W, C_BORDER);
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
    case PAGE_HELP:
      uiDrawPageHelp();
      break;
    case PAGE_SD_EXPLORER:
      uiDrawPageSdExplorer();
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

  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("WALL-E", 10, 6);
    uiDrawControlAuthority(packetTelemetryValid());
  }
  
  // Force initial telemetry and control authority draw
  s_lastBatV = -999.0f;
  s_lastModeStr = nullptr;
  s_lastPacketRate = 9999;
  s_lastEmoStr[0] = '\0';
  
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

  g_tft->drawFastHLine(0, BOTTOM_BAR_Y, SCREEN_W, C_BORDER);
  g_tft->fillRect(0, BOTTOM_BAR_Y, SCREEN_W, BOTTOM_BAR_H, C_BG_DARK);

  const int by = BOTTOM_BAR_Y + 4, bh = 32;
  g_tft->fillRoundRect(DRIVE_DOCK_X, by, DRIVE_DOCK_W, bh, 3, C_ACCENT);
  g_tft->drawRoundRect(DRIVE_DOCK_X, by, DRIVE_DOCK_W, bh, 3, C_ACCENT_DIM);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->setTextSize(1);
  g_tft->drawString("Dock", DRIVE_DOCK_X + 14, by + 10);
  g_tft->fillRoundRect(DRIVE_CANCEL_X, by, DRIVE_CANCEL_W, bh, 3, C_BG_DARK);
  g_tft->drawRoundRect(DRIVE_CANCEL_X, by, DRIVE_CANCEL_W, bh, 3, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->drawString("Cancel", DRIVE_CANCEL_X + 6, by + 10);

  g_tft->fillRoundRect(DRIVE_ESTOP_X, by, DRIVE_ESTOP_W, bh, 4, C_RED);
  g_tft->drawRoundRect(DRIVE_ESTOP_X, by, DRIVE_ESTOP_W, bh, 4, C_WHITE);
  g_tft->setTextColor(C_WHITE, C_RED);
  g_tft->setTextSize(2);
  g_tft->drawString("E-STOP", DRIVE_ESTOP_X + 4, by + 8);

  g_tft->setTextSize(1);
  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->drawRoundRect(DRIVE_NAV_GRID_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Sys", DRIVE_NAV_GRID_X + 3, by + 10);
  g_tft->drawRoundRect(DRIVE_NAV_BEH_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Beh", DRIVE_NAV_BEH_X + 3, by + 10);
  g_tft->drawRoundRect(DRIVE_NAV_PRF_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Prf", DRIVE_NAV_PRF_X + 4, by + 10);
  g_tft->drawRoundRect(DRIVE_NAV_AUT_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Aut", DRIVE_NAV_AUT_X + 4, by + 10);
}

void uiDrawStaticBehaviour(void) {
  uiDrawPageBehaviour();
}

void uiDrawStaticSystem(void) {
  uiDrawPageSystem();
}

void uiPhysFavouriteCellPos(int index, int* outBx, int* outBy) {
  const int cTop = uiContentTop();
  const int midX = SCREEN_W / 2;
  const int rightW = SCREEN_W - midX;
  const int gridW = PHYS_FAV_BOX_W * 2 + PHYS_FAV_COL_GAP;
  const int favBaseX = midX + (rightW - gridW) / 2;
  const int favBlockH = 2 * PHYS_FAV_ROW_STEP + PHYS_FAV_BOX_H;
  const int availH = BOTTOM_BAR_Y - cTop - PHYS_FAV_TOP_PAD - PHYS_FAV_BOTTOM_PAD;
  int favBaseY = cTop + PHYS_FAV_TOP_PAD;
  if (availH > favBlockH) {
    favBaseY += (availH - favBlockH) / 2;
  }
  int i = index;
  if (i < 0) i = 0;
  if (i > 5) i = 5;
  if (outBx) {
    *outBx = favBaseX + (i % 2) * (PHYS_FAV_BOX_W + PHYS_FAV_COL_GAP);
  }
  if (outBy) {
    *outBy = favBaseY + (i / 2) * PHYS_FAV_ROW_STEP;
  }
}

#if USE_PHYSICAL_JOYSTICKS
void uiDrawPhysicalJoystickLayout(void) {
  const int cTop = uiContentTop();
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("WALL-E Console", 10, 6);
    uiDrawControlAuthority(packetTelemetryValid());
  }

  int midX = SCREEN_W / 2;
  g_tft->drawFastVLine(midX, cTop, uiContentHeight(), C_BORDER);
  /* "Battery" / "Behaviour" labels drawn in uiDrawTelemetryStrip (telemetry row) */

  // Get animation names based on current profile's favorites
  Profile* p = profileGet();
  static const char kAnimShortNames[][9] = {
    "Reset", "Bootup", "Inquis", "BrowR", "BrowL", "Suprs",
    "Nod", "LookSd", "Wave", "Sleepy", "Shake", "Perk",
    "Sad", "Tilt", "LeanIn", "Peek", "Yawn", "Jump",
    "Wiggle", "Cheer", "Shy", "Up", "Down", "Fidget"
  };
  const char* displayNames[6];

  for (int i = 0; i < 6; i++) {
    uint8_t animId = p->favoriteAnimations[i];
    if (animId < ANIMATION_COUNT) {
      displayNames[i] = kAnimShortNames[animId];
    } else {
      displayNames[i] = "---";
    }
  }

  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->setTextSize(1);
  const int animCharW = 6;
  for (int i = 0; i < 6; i++) {
    int bx, by;
    uiPhysFavouriteCellPos(i, &bx, &by);
    g_tft->drawRect(bx, by, PHYS_FAV_BOX_W, PHYS_FAV_BOX_H, C_BORDER);
    const char* name = displayNames[i];
    int tw = (int)strlen(name) * animCharW;
    if (tw > PHYS_FAV_BOX_W - 4) tw = PHYS_FAV_BOX_W - 4;
    int tx = bx + (PHYS_FAV_BOX_W - tw) / 2;
    if (tx < bx + 2) tx = bx + 2;
    int ty = by + (PHYS_FAV_BOX_H - 8) / 2;
    g_tft->drawString(name, tx, ty);
  }
  
  g_tft->drawFastHLine(0, BOTTOM_BAR_Y, SCREEN_W, C_BORDER);
  g_tft->fillRect(0, BOTTOM_BAR_Y, SCREEN_W, BOTTOM_BAR_H, C_BG_DARK);
  const int by = BOTTOM_BAR_Y + 4, bh = 32;
  g_tft->fillRoundRect(DRIVE_DOCK_X, by, DRIVE_DOCK_W, bh, 3, C_ACCENT);
  g_tft->drawRoundRect(DRIVE_DOCK_X, by, DRIVE_DOCK_W, bh, 3, C_ACCENT_DIM);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->setTextSize(1);
  g_tft->drawString("Dock", DRIVE_DOCK_X + 14, by + 10);
  g_tft->fillRoundRect(DRIVE_CANCEL_X, by, DRIVE_CANCEL_W, bh, 3, C_BG_DARK);
  g_tft->drawRoundRect(DRIVE_CANCEL_X, by, DRIVE_CANCEL_W, bh, 3, C_BORDER);
  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->drawString("Cancel", DRIVE_CANCEL_X + 6, by + 10);
  g_tft->fillRoundRect(DRIVE_ESTOP_X, by, DRIVE_ESTOP_W, bh, 4, C_RED);
  g_tft->drawRoundRect(DRIVE_ESTOP_X, by, DRIVE_ESTOP_W, bh, 4, C_WHITE);
  g_tft->setTextColor(C_WHITE, C_RED);
  g_tft->setTextSize(2);
  g_tft->drawString("E-STOP", DRIVE_ESTOP_X + 4, by + 8);
  g_tft->setTextSize(1);
  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->drawRoundRect(DRIVE_NAV_GRID_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Sys", DRIVE_NAV_GRID_X + 3, by + 10);
  g_tft->drawRoundRect(DRIVE_NAV_BEH_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Beh", DRIVE_NAV_BEH_X + 3, by + 10);
  g_tft->drawRoundRect(DRIVE_NAV_PRF_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Prf", DRIVE_NAV_PRF_X + 4, by + 10);
  g_tft->drawRoundRect(DRIVE_NAV_AUT_X, by, DRIVE_NAV_CELL_W, bh, 2, C_BORDER);
  g_tft->drawString("Aut", DRIVE_NAV_AUT_X + 4, by + 10);
  g_tft->drawFastHLine(0, BOTTOM_BAR_Y, SCREEN_W, C_BORDER);
  /* Laser toggle is drawn last each frame (see uiRenderingDrawDriveLaserOverlayIfNeeded)
   * so joystick/eye overlays cannot paint over it. */
}
#endif

// ------------------------------------------------------------
//  Telemetry strip (battery bar, V, temp, current, pkt/s, mode)
// ------------------------------------------------------------
void uiDrawTelemetryStrip(const TelemetryStripData* telem) {
  if (!g_tft || !telem) return;

  if (g_topBannerCollapsed) {
    int batPct = telem->batteryPct;
    if (batPct < 0) batPct = 0;
    if (batPct > 100) batPct = 100;
    const char* emo = telem->emotionStr ? telem->emotionStr : "";
    bool changed = (telem->batteryV != s_lastBatV || batPct != s_lastBatPct ||
                    telem->currentA != s_lastCurrent || telem->tempC != s_lastTemp ||
                    telem->packetRate != s_lastPacketRate || telem->modeStr != s_lastModeStr ||
                    strcmp(s_lastEmoStr, emo) != 0);
    if (!changed) return;
    s_lastBatV = telem->batteryV;
    s_lastBatPct = batPct;
    s_lastCurrent = telem->currentA;
    s_lastTemp = telem->tempC;
    s_lastPacketRate = telem->packetRate;
    s_lastModeStr = telem->modeStr;
    strncpy(s_lastEmoStr, emo, sizeof(s_lastEmoStr) - 1);
    s_lastEmoStr[sizeof(s_lastEmoStr) - 1] = '\0';

    g_tft->fillRect(0, 0, SCREEN_W, BANNER_MINI_H, C_BG_DARK);
    g_tft->drawFastHLine(0, BANNER_MINI_H - 1, SCREEN_W, C_BORDER);
    char line[44];
    const char* mode = telem->modeStr ? telem->modeStr : "--";
    snprintf(line, sizeof(line), "%d%% %.1fV %u/s %s", batPct,
             (double)telem->batteryV, (unsigned)telem->packetRate, mode);
    if (strlen(line) > 38) line[38] = '\0';
    g_tft->setTextColor(telem->connected ? C_WHITE : C_TEXT_DIM, C_BG_DARK);
    g_tft->setTextSize(1);
    g_tft->setCursor(4, 4);
    g_tft->print(line);
    g_tft->setTextColor(C_ACCENT, C_BG_DARK);
    g_tft->setCursor(SCREEN_W - 44, 4);
    g_tft->print("v");
    return;
  }

  int w = 60, h = 8;
  int batPct = telem->batteryPct;
  if (batPct < 0) batPct = 0;
  if (batPct > 100) batPct = 100;

  const char* emo = telem->emotionStr ? telem->emotionStr : "";
  bool changed = (telem->batteryV != s_lastBatV || batPct != s_lastBatPct ||
                  telem->currentA != s_lastCurrent || telem->tempC != s_lastTemp ||
                  telem->packetRate != s_lastPacketRate || telem->modeStr != s_lastModeStr ||
                  strcmp(s_lastEmoStr, emo) != 0);
  if (!changed) return;

  s_lastBatV = telem->batteryV;
  s_lastBatPct = batPct;
  s_lastCurrent = telem->currentA;
  s_lastTemp = telem->tempC;
  s_lastPacketRate = telem->packetRate;
  s_lastModeStr = telem->modeStr;
  strncpy(s_lastEmoStr, emo, sizeof(s_lastEmoStr) - 1);
  s_lastEmoStr[sizeof(s_lastEmoStr) - 1] = '\0';

  g_tft->fillRect(0, TOP_BAR_HEIGHT, SCREEN_W, TELEM_STRIP_H, C_BG_DARK);
  /* Row 1: labels under title bar, above the % bar */
  g_tft->setTextSize(1);
  g_tft->setTextColor(C_ACCENT, C_BG_DARK);
  g_tft->drawString("Battery", 10, TOP_BAR_HEIGHT + 2);
#if USE_PHYSICAL_JOYSTICKS
  if (g_inputMode == INPUT_PHYSICAL_JOYSTICK && g_currentPage == PAGE_DRIVE) {
    g_tft->drawString("Behaviour", SCREEN_W / 2 + 8, TOP_BAR_HEIGHT + 2);
  }
#endif
  const int yBar = TOP_BAR_HEIGHT + 11;
  g_tft->drawRect(10, yBar, w + 2, h + 2, C_BORDER);
  g_tft->fillRect(11, yBar + 1, (w * batPct) / 100, h, telem->connected ? C_GREEN : C_ACCENT_DIM);
  g_tft->fillRect(11 + (w * batPct) / 100, yBar + 1, w - (w * batPct) / 100, h, C_BG);

  char batText[8];
  snprintf(batText, sizeof(batText), "%d%%", batPct);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawString(batText, 14, yBar + 1);

  /* 320px wide: one long line from x=78 clips after ~40 chars — use two lines */
  char line1[48];
  char line2[44];
  snprintf(line1, sizeof(line1), "%.1fV %.0fC %.1fA %up/s",
           telem->batteryV, telem->tempC, telem->currentA,
           (unsigned)telem->packetRate);
  const char* mode = telem->modeStr ? telem->modeStr : "--";
  snprintf(line2, sizeof(line2), "%s | %s", mode, emo[0] ? emo : "--");
  if (strlen(line2) > 39) line2[39] = '\0';

  const int yLine2 = TOP_BAR_HEIGHT + 20;
  g_tft->setTextColor(telem->connected ? C_WHITE : C_TEXT_DIM, C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawString(line1, 78, yBar + 1);
  g_tft->drawString(line2, 78, yLine2);
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

void uiDrawControlAuthority(bool brainLinkOk) {
  if (!g_tft) return;
  const int y = 4;
  uint16_t color = C_GREEN;
  const char* label = "LOCAL";
  bool pulse = false;
  uint16_t pulseColor = C_BLUE;

  /* E-STOP / safety overrides link. Otherwise no Brain telemetry ⇒ OFFLINE (not LOCAL). */
  if (g_controlAuthority == CTRL_SAFETY) {
    color = C_RED;
    label = "SAFE";
    pulse = true;
    pulseColor = C_RED;
  } else if (!brainLinkOk) {
    color = C_TEXT_DIM;
    label = "OFFLINE";
    pulse = true;
    pulseColor = C_RED;
  } else if (g_policyDenyCyd) {
    color = 0xFDA0U; /* amber */
    label = "POLICY";
    pulse = true;
    pulseColor = 0xFDA0U;
  } else if (g_motionPolicyFromBrain == 2) {
    color = C_YELLOW;
    label = "WEB";
    pulse = true;
    pulseColor = C_YELLOW;
  } else if (g_motionPolicyFromBrain == 1) {
    color = C_GREEN;
    label = "DESK";
  } else {
    switch (g_controlAuthority) {
      case CTRL_AUTONOMOUS: color = C_BLUE;  label = "AUTO"; pulse = true; pulseColor = C_BLUE;  break;
      case CTRL_SUPERVISED: color = C_YELLOW; label = "SUPV"; pulse = true; pulseColor = C_YELLOW; break;
      default: break; /* LOCAL */
    }
  }
  float brightness = animGetPulseBrightness();
  uint16_t bg = pulse ? lerpRGB(C_BG_DARK, pulseColor, brightness * 0.25f) : C_BG_DARK;

  /* GLCD font @ size 1 ≈ 6px/char — always wipe the full top-right slot so shorter labels
   * (e.g. SAFE) do not leave ghost pixels from longer ones (OFFLINE), which looked like
   * duplicate or overlapped "CTRL:…" text. */
  const int slotW = 120;
  const int slotH = 12;
  const int slotX = SCREEN_W - slotW;
  const int charW = 6;
  const char kCtrl[] = "CTRL:";
  int wCtrl = (int)strlen(kCtrl) * charW;
  int wLab = (int)strlen(label) * charW;
  int textW = wCtrl + wLab;
  int textX = slotX + (slotW - textW) / 2;
  if (textX < slotX) textX = slotX;

  g_tft->fillRect(slotX, y, slotW, slotH, bg);

  g_tft->setTextColor(color, bg);
  g_tft->setTextSize(1);
  g_tft->drawString(kCtrl, textX, y);
  g_tft->drawString(label, textX + wCtrl, y);
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
  if (!g_topBannerCollapsed) {
    uiDrawControlAuthority(telem && telem->connected);
  }
}

void uiDrawEStopRegion(bool highlighted) {
  if (!g_tft) return;
  int ex = DRIVE_ESTOP_X, ey = BOTTOM_BAR_Y + 4, ew = DRIVE_ESTOP_W, eh = 32;
  g_tft->fillRoundRect(ex, ey, ew, eh, 4, highlighted ? 0xFF00 : C_RED);
  g_tft->drawRoundRect(ex, ey, ew, eh, 4, C_WHITE);
  g_tft->setTextColor(C_WHITE, highlighted ? 0xFF00 : C_RED);
  g_tft->setTextSize(2);
  g_tft->drawString("E-STOP", ex + 4, ey + 8);
}

void uiDrawPageBehaviour(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);
  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("Animations", 10, 6);
  }

  static const char kAnimShortNames[][9] = {
    "Reset", "Bootup", "Inquis", "BrowR", "BrowL", "Suprs",
    "Nod", "LookSd", "Wave", "Sleepy", "Shake", "Perk",
    "Sad", "Tilt", "LeanIn", "Peek", "Yawn", "Jump",
    "Wiggle", "Cheer", "Shy", "Up", "Down", "Fidget"
  };

  Profile* p = profileGet();
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->setTextSize(1);

  const int behTop = uiContentTop() + 10;

  auto drawAnimCell = [&](int animId, int bx, int by, int bw, int bh) {
    bool isFavorite = false;
    for (int f = 0; f < 6; f++) {
      if (p->favoriteAnimations[f] == (uint8_t)animId) {
        isFavorite = true;
        break;
      }
    }
    uint16_t borderColor = isFavorite ? TFT_YELLOW : C_BORDER;
    g_tft->drawRect(bx, by, bw, bh, borderColor);
    if (isFavorite) {
      g_tft->drawRect(bx + 1, by + 1, bw - 2, bh - 2, borderColor);
    }
    const char* label = (animId >= 0 && animId < ANIMATION_COUNT)
                            ? kAnimShortNames[animId]
                            : "?";
    g_tft->setTextColor(C_ACCENT, C_BG);
    g_tft->drawString(label, bx + 6, by + 12);
    char idBuf[6];
    snprintf(idBuf, sizeof(idBuf), "%d", animId);
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->drawString(idBuf, bx + bw - 18, by + 12);
    if (isFavorite) {
      g_tft->setTextColor(TFT_YELLOW, C_BG);
      g_tft->drawString("*", bx + bw - 18, by + 22);
    }
  };

  for (int i = 0; i < 6; i++) {
    int animId = (int)g_behaviourAnimPage * 6 + i;
    if (animId >= ANIMATION_COUNT) break;
    int bx = 16 + (i % 3) * 100;
    int by = behTop + (i / 3) * 50;
    drawAnimCell(animId, bx, by, 90, 36);
  }

  /* Small centered page strip (above hint / Back) */
  {
    const int pgX = BEHAV_PAGE_BTN_X;
    const int pgY = BEHAV_PAGE_BTN_Y;
    const int pgW = BEHAV_PAGE_BTN_W;
    const int pgH = BEHAV_PAGE_BTN_H;
    g_tft->fillRoundRect(pgX, pgY, pgW, pgH, 2, C_BG_DARK);
    g_tft->drawRoundRect(pgX, pgY, pgW, pgH, 2, C_BORDER);
    g_tft->setTextColor(C_ACCENT, C_BG_DARK);
    g_tft->setTextSize(1);
    char pgLab[12];
    snprintf(pgLab, sizeof(pgLab), "Pg %u/4", (unsigned)g_behaviourAnimPage + 1u);
    const int tw = (int)strlen(pgLab) * 6;
    int tx = pgX + (pgW - tw) / 2;
    if (tx < pgX + 2) tx = pgX + 2;
    g_tft->setCursor(tx, pgY + (pgH - 8) / 2);
    g_tft->print(pgLab);
  }

  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  g_tft->drawString("Tap play | Hold fav", 8, BOTTOM_BAR_Y - 10);

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
  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("System", 10, 6);
  }
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  /* Motion policy — tap row cycles Any → Desk → Web (ESP-NOW to Base); touch_input SYS 40..67 */
  {
    const int mpX = 8, mpY = 40, mpW = 304, mpH = 28;
    g_tft->fillRect(mpX, mpY, mpW, mpH, C_BG_DARK);
    g_tft->drawRect(mpX, mpY, mpW, mpH, C_BORDER);
    const char* mp = "ANY  (LROS + desk)";
    if (g_motionPolicyFromBrain == 1) mp = "DESK only";
    else if (g_motionPolicyFromBrain == 2) mp = "WEB only";
    g_tft->setTextColor(g_policyDenyCyd ? C_YELLOW : C_WHITE, C_BG_DARK);
    g_tft->setCursor(14, mpY + 6);
    g_tft->print("Motion: ");
    g_tft->print(mp);
    g_tft->setTextColor(C_ACCENT, C_BG_DARK);
    g_tft->setCursor(248, mpY + 6);
    g_tft->print("TAP");
  }
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->drawString("Battery: -- V", 16, 74);
  g_tft->drawString("Current: -- A", 16, 98);
  g_tft->drawString("Temp: -- C", 16, 122);
  
  // Profiles button
  g_tft->fillRect(16, 130, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Profiles", 32, 140);
  
  // Servo Test button
  g_tft->fillRect(130, 130, 100, 32, C_ACCENT);
  g_tft->drawString("Servos", 150, 140);
  
  // Row 2: Autonomy | Help | SD (touch zones must match touch_input page 2)
  g_tft->fillRect(4, 168, 100, 32, C_GREEN);
  g_tft->setTextColor(C_WHITE, C_GREEN);
  g_tft->drawString("Autonomy", 16, 178);
  g_tft->fillRect(110, 168, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Help", 138, 178);
  g_tft->fillRect(216, 168, 100, 32, C_YELLOW);
  g_tft->setTextColor(C_BG, C_YELLOW);
  g_tft->drawString("Mem", 244, 178);
  
  // Back button
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

// ============================================================
//  Autonomy Page — Live telemetry + Tune (remote config to Base)
// ============================================================
/* Tune row layout — labels left, − / value / + ; touch_input PAGE_AUTONOMY must match x */
#define AU_TUNE_X_MINUS   84
#define AU_TUNE_X_VAL    124
#define AU_TUNE_X_PLUS   264
#define AU_TUNE_BTN_W     36

static void drawAutonomyTuneRow(const char* label, int y, uint8_t val, const char* suffix) {
  if (!g_tft) return;
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  g_tft->drawString(label, 4, y + 3);
  g_tft->fillRect(AU_TUNE_X_MINUS, y, AU_TUNE_BTN_W, 16, C_BG_DARK);
  g_tft->drawRect(AU_TUNE_X_MINUS, y, AU_TUNE_BTN_W, 16, C_BORDER);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setCursor(AU_TUNE_X_MINUS + 12, y + 4);
  g_tft->print("-");
  char vb[16];
  snprintf(vb, sizeof(vb), "%u%s", (unsigned)val, suffix);
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->setCursor(AU_TUNE_X_VAL, y + 3);
  g_tft->print(vb);
  g_tft->fillRect(AU_TUNE_X_PLUS, y, AU_TUNE_BTN_W, 16, C_BG_DARK);
  g_tft->drawRect(AU_TUNE_X_PLUS, y, AU_TUNE_BTN_W, 16, C_BORDER);
  g_tft->setTextColor(C_WHITE, C_BG_DARK);
  g_tft->setCursor(AU_TUNE_X_PLUS + 12, y + 4);
  g_tft->print("+");
}

void uiDrawPageAutonomy(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);
  
  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("Autonomy", 10, 6);
  }
  
  TelemetryPacket telem;
  packetGetTelemetry(&telem);

  const int c = uiContentTop();
  /* Tabs — must match touch_input PAGE_AUTONOMY */
  g_tft->fillRect(4, c + 4, 150, 22, g_autonomyUiTab == 0 ? C_ACCENT : C_BG_DARK);
  g_tft->drawRect(4, c + 4, 150, 22, C_BORDER);
  g_tft->setTextColor(g_autonomyUiTab == 0 ? C_BG : C_ACCENT, g_autonomyUiTab == 0 ? C_ACCENT : C_BG_DARK);
  g_tft->setTextSize(1);
  g_tft->drawString("Status", 58, c + 10);
  g_tft->fillRect(166, c + 4, 150, 22, g_autonomyUiTab == 1 ? C_ACCENT : C_BG_DARK);
  g_tft->drawRect(166, c + 4, 150, 22, C_BORDER);
  g_tft->setTextColor(g_autonomyUiTab == 1 ? C_BG : C_ACCENT, g_autonomyUiTab == 1 ? C_ACCENT : C_BG_DARK);
  g_tft->drawString("Adjust", 220, c + 10);

  const int body = c + 30;
  char buf[48];

  if (g_autonomyUiTab == 0) {
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->setCursor(4, body);
    g_tft->print("Allow robot self-drive");
    uint16_t armCol = g_remoteAutonomyArm ? C_GREEN : C_RED;
    g_tft->fillRect(200, body + 2, 110, 28, armCol);
    g_tft->setTextColor(C_WHITE, armCol);
    g_tft->setTextSize(1);
    g_tft->drawString(g_remoteAutonomyArm ? "ON" : "OFF", 236, body + 10);

    snprintf(buf, sizeof(buf), "Brain auto: %s", telem.autonomyEnabled ? "running" : "off");
    g_tft->setTextColor(C_ACCENT, C_BG);
    g_tft->drawString(buf, 4, body + 36);
    snprintf(buf, sizeof(buf), "Behavior: %s", getAutonomyStateName(telem.autonomyState));
    g_tft->drawString(buf, 4, body + 52);
    snprintf(buf, sizeof(buf), "Front distance: %.0f cm", telem.sonarDistanceCm);
    g_tft->drawString(buf, 4, body + 68);
    snprintf(buf, sizeof(buf), "Compass: %.0f deg", telem.compassHeading);
    g_tft->drawString(buf, 4, body + 84);
    if (telem.gpsValid) {
      snprintf(buf, sizeof(buf), "GPS: locked");
    } else {
      snprintf(buf, sizeof(buf), "GPS: searching");
    }
    g_tft->drawString(buf, 4, body + 100);
    if (telem.waypointMode) {
      snprintf(buf, sizeof(buf), "Route pt %u/%u  %.1f m", (unsigned)(telem.currentWaypoint + 1),
               (unsigned)telem.totalWaypoints, telem.waypointDistanceM);
      g_tft->drawString(buf, 4, body + 116);
    }
  } else {
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->setCursor(4, body - 2);
    g_tft->print("Send to robot over radio");
    int y = body + 8;
    const int rh = 16;
    /* Sonar thresholds: “react” = backup/avoid, “look at” = investigate */
    drawAutonomyTuneRow("React dist", y, g_auCloseCm, "cm");
    y += rh;
    drawAutonomyTuneRow("Look dist", y, g_auInterestCm, "cm");
    y += rh;
    drawAutonomyTuneRow("Curiosity", y, g_auCuriosityPct, "%");
    y += rh;
    drawAutonomyTuneRow("Bravery", y, g_auBraveryPct, "%");
    y += rh;
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->drawString("Follow GPS route", 4, y + 3);
    g_tft->fillRect(128, y, 72, 16, g_auWaypointFollow ? C_GREEN : C_BG_DARK);
    g_tft->drawRect(128, y, 72, 16, C_BORDER);
    g_tft->setTextColor(C_WHITE, g_auWaypointFollow ? C_GREEN : C_BG_DARK);
    g_tft->setCursor(140, y + 4);
    g_tft->print(g_auWaypointFollow ? "ON" : "OFF");
    y += rh + 4;
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->drawString("Personality", 4, y + 2);
    const char* plab[] = {"Careful", "Balanced", "Explorer", "Playful"};
    for (int i = 0; i < 4; i++) {
      int bx = 52 + i * 66;
      g_tft->fillRect(bx, y, 62, 16, C_BG_DARK);
      g_tft->drawRect(bx, y, 62, 16, C_BORDER);
      g_tft->setTextColor(C_ACCENT, C_BG_DARK);
      int tw = (int)strlen(plab[i]) * 6;
      int tx = bx + (62 - tw) / 2;
      if (tx < bx + 2) tx = bx + 2;
      g_tft->setCursor(tx, y + 4);
      g_tft->print(plab[i]);
    }
  }
  
  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

// ============================================================
//  Help — topic index + short reference pages
// ============================================================
void uiDrawPageHelp(void) {
  if (!g_tft) return;
  g_tft->fillScreen(C_BG);
  for (int x = 0; x < SCREEN_W; x += GRID_SPACING)
    g_tft->drawFastVLine(x, 0, SCREEN_H, C_GRID);
  for (int y = 0; y < SCREEN_H; y += GRID_SPACING)
    g_tft->drawFastHLine(0, y, SCREEN_W, C_GRID);
  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("Help", 10, 6);
  }

  const int c = uiContentTop();

  if (g_helpSection == 0) {
    g_tft->setTextColor(C_TEXT_DIM, C_BG);
    g_tft->setTextSize(1);
    g_tft->setCursor(8, c + 4);
    g_tft->print("Choose a topic:");
    const char* labs[] = {
      "Drive & safety",
      "Self-drive & autonomy",
      "Animations",
      "System & radio link"
    };
    for (int i = 0; i < 4; i++) {
      int y = c + 22 + i * 38;
      g_tft->fillRect(10, y, 300, 32, C_BG_DARK);
      g_tft->drawRect(10, y, 300, 32, C_BORDER);
      g_tft->setTextColor(C_ACCENT, C_BG_DARK);
      int tw = (int)strlen(labs[i]) * 6;
      int tx = 10 + (300 - tw) / 2;
      if (tx < 14) tx = 14;
      g_tft->setCursor(tx, y + 12);
      g_tft->print(labs[i]);
    }
  } else {
    const char* title = "?";
    const char* lines[10];
    int n = 0;
    switch (g_helpSection) {
      case 1:
        title = "Drive & safety";
        lines[n++] = "Joystick / arrows move tracks.";
        lines[n++] = "E-STOP cuts motor power at once.";
        lines[n++] = "Dock & Cancel control charging";
        lines[n++] = "homing when the Brain supports it.";
        lines[n++] = "Bottom row: Sys, Beh, profiles,";
        lines[n++] = "Autonomy. CTRL shows who steers.";
        lines[n++] = "You need a radio link for drive.";
        break;
      case 2:
        title = "Self-drive";
        lines[n++] = "Status: allow robot self-drive.";
        lines[n++] = "Adjust: send tuning over radio.";
        lines[n++] = "React / Look = sonar distances.";
        lines[n++] = "Curiosity & Bravery shape style.";
        lines[n++] = "Personality = quick presets.";
        lines[n++] = "Follow GPS = route following.";
        break;
      case 3:
        title = "Animations";
        lines[n++] = "Tap a tile to play on the robot.";
        lines[n++] = "Hold to star a favorite.";
        lines[n++] = "Favorites show a second border.";
        break;
      case 4:
        title = "System & link";
        lines[n++] = "Profiles store servo curves.";
        lines[n++] = "Servos opens the servo test page.";
        lines[n++] = "Autonomy opens self-drive tools.";
        lines[n++] = "OFFLINE = no recent Brain link.";
        lines[n++] = "Tap banner edge to shrink status.";
        break;
      default:
        title = "Help";
        lines[n++] = "Use Back to return.";
        break;
    }
    if (n > 0) {
      g_tft->setTextColor(C_ACCENT, C_BG);
      g_tft->setTextSize(2);
      g_tft->drawString(title, 8, c + 4);
      g_tft->setTextSize(1);
      g_tft->setTextColor(C_TEXT_DIM, C_BG);
      for (int i = 0; i < n; i++) {
        g_tft->setCursor(8, c + 32 + i * 14);
        g_tft->print(lines[i]);
      }
      g_tft->setTextColor(C_TEXT_DIM, C_BG);
      g_tft->setCursor(8, BOTTOM_BAR_Y - 16);
      g_tft->print("Back = topic list");
    }
  }

  g_tft->fillRect(SCREEN_W / 2 - 50, BOTTOM_BAR_Y + 4, 100, 32, C_ACCENT);
  g_tft->setTextColor(C_BG, C_ACCENT);
  g_tft->drawString("Back", SCREEN_W / 2 - 18, BOTTOM_BAR_Y + 14);
}

// ------------------------------------------------------------
//  Bottom toast: Base autonomy in a “busy mind” state
// ------------------------------------------------------------
void uiDrawThinkingStrip(const TelemetryPacket* tm, bool linkOk) {
  if (!g_tft) return;

  static uint8_t s_wasOn = 0;

  const char* line = nullptr;
  if (linkOk && tm && tm->autonomyEnabled) {
    switch (tm->autonomyState) {
      case 1:  line = "WALL·E is looking around…"; break;
      case 2:  line = "WALL·E is thinking…"; break;
      case 4:  line = "WALL·E is inspecting…"; break;
      case 8:  line = "WALL·E is finding the way…"; break;
      default: break;
    }
  }

  if (!line) {
    if (s_wasOn) {
      g_needStaticRedraw = true;
      s_wasOn = 0;
    }
    return;
  }
  s_wasOn = 1;

  const int y = UI_THINKING_STRIP_Y;
  const int h = UI_THINKING_STRIP_H;
  const uint16_t panel = 0x2103;
  g_tft->fillRoundRect(4, y, SCREEN_W - 8, h, 5, panel);
  g_tft->drawRoundRect(4, y, SCREEN_W - 8, h, 5, C_ACCENT_DIM);
  g_tft->setTextSize(1);
  g_tft->setTextColor(C_ACCENT, panel);
  int lw = (int)strlen(line) * 6;
  int tx = (SCREEN_W - lw) / 2;
  if (tx < 6) tx = 6;
  g_tft->setCursor(tx, y + 9);
  g_tft->print(line);
}

// ------------------------------------------------------------
//  Profile Selection Page
// ------------------------------------------------------------
void uiDrawStaticProfile(void) {
  if (!g_tft) return;
  
  g_tft->fillScreen(C_BG);
  
  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("Profile", 10, 6);
  }
  
  // Profile cards (3 profiles)
  const int cardW = 90;
  const int cardH = 120;
  const int cardSpacing = 10;
  const int startX = (SCREEN_W - (cardW * 3 + cardSpacing * 2)) / 2;
  const int startY = uiContentTop() + 10;
  
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
  
  uiDrawBannerBackground();
  Profile* p = profileGet();
  char titleBuf[32];
  snprintf(titleBuf, sizeof(titleBuf), "%s Tuning", p->name);
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString(titleBuf, 10, 6);
  }
  
  // Adjustment sliders
  const int startY = uiContentTop() + 8;
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
  
  uiDrawBannerBackground();
  if (!g_topBannerCollapsed) {
    g_tft->setTextColor(C_WHITE, C_BG_DARK);
    g_tft->setTextSize(2);
    g_tft->drawString("Servo Test", 10, 6);
  }
  
  // Get current servo positions
  uint8_t servoTargets[SERVO_COUNT];
  motionGetServoTargets(servoTargets);
  
  // Servo names
  const char* servoNames[SERVO_COUNT] = {
    "Pan", "Tilt", "EyeL", "EyeR", "NeckT",
    "NeckB", "ArmL", "ArmR", "BrowR", "BrowL"
  };
  
  // Draw servo sliders (2 columns)
  const int startY = uiContentTop() + 4;
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
  int x = 4, y = uiContentTop() + 4;
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
//  HEAD and DRIVE mini joysticks in middle-left (left of centre divider)
// ------------------------------------------------------------
void uiDrawPhysicalJoystickIndicators(float joy1X, float joy1Y, float joy2X, float joy2Y) {
  if (!g_tft) return;
  
  // Middle left: HEAD and DRIVE side by side with a gap
  const int joy1_cx = 25;
  const int joy1_cy = 105;
  const int joy2_cx = 115;
  const int joy2_cy = 105;
  const int joy_radius = 25;
  const int stick_radius = 6;
  
  // Clear previous indicators
  static int lastJoy1X = 0, lastJoy1Y = 0;
  static int lastJoy2X = 0, lastJoy2Y = 0;
  
  // Draw Joy1 base (left - head control)
  g_tft->drawCircle(joy1_cx, joy1_cy, joy_radius, C_ACCENT_DIM);
  g_tft->drawCircle(joy1_cx, joy1_cy, 2, C_ACCENT_DIM); // Center dot
  g_tft->setTextColor(C_TEXT_DIM, C_BG);
  g_tft->setTextSize(1);
  g_tft->drawString("HEAD", joy1_cx - 15, joy1_cy - 32);
  
  // Draw Joy2 base (tank drive)
  g_tft->drawCircle(joy2_cx, joy2_cy, joy_radius, C_ACCENT);
  g_tft->drawCircle(joy2_cx, joy2_cy, 2, C_ACCENT); // Center dot
  g_tft->setTextColor(C_ACCENT, C_BG);
  g_tft->drawString("DRIVE", joy2_cx - 17, joy2_cy - 32);
  
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
