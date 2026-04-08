// ============================================================
//  WALL-E Master Controller — Touch Input Implementation
//  Proper joystick math: deadzone, radial clamping, smoothing
// ============================================================

#include <Arduino.h>
#include <SPI.h>
#include <math.h>

#include "touch_input.h"
#include "ui_buttons.h"
#include "motion_engine.h"  // For SERVO_COUNT
#include "cyd_laser_ui.h"
#include "ui_draw.h"
#include "ui_state.h"
#include "ui_sd_explorer.h"
#include "ui_sd_memory_core.h"
#include <XPT2046_Touchscreen.h>

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

/* SPI3 on classic ESP32 (same as legacy VSPI macro). Arduino-ESP32 3.x may not
 * expose VSPI until after full target headers; numeric id avoids that. Other
 * ESP32 variants: use default HSPI bus for a second SPI peripheral. */
#if CONFIG_IDF_TARGET_ESP32
static SPIClass s_touchSPI(3);
#else
static SPIClass s_touchSPI(HSPI);
#endif
static XPT2046_Touchscreen s_ts(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_W 320
#define SCREEN_H 240
#define QUICK_ZONE_X_MIN 270
#define QUICK_ZONE_Y_MIN 210
#define ADVANCED_ZONE_X_MAX 50
#define ADVANCED_ZONE_Y_MAX 50

// Drive joystick geometry: JOY_CX / JOY_CY / JOY_RADIUS from ui_draw.h
#define JOY_DEADZONE ((float)(JOY_RADIUS * JOY_DEADZONE_PCT) / 100.0f)
#define JOY_SMOOTH_PREV (1.0f - JOY_SMOOTH_FACTOR)

static DriveState s_driveState;
static DriveState s_smoothedState;
static unsigned long s_lastInputMs = 0;
static Direction s_currentDirection = DIR_NONE;  // Current button direction

// Long-press (2s bottom-right corner)
static unsigned long s_pressStartMs = 0;
static bool s_pressInQuickZone = false;
static bool s_quickActionFired = false;

// Animation long-press for favoriting (500ms)
static unsigned long s_animPressStartMs = 0;
static int8_t s_animPressZone = -1;
static bool s_animLongPressFired = false;

// Triple-tap top-left (count on release when finger was in zone during press)
static int s_tapCount = 0;
static unsigned long s_tapWindowStartMs = 0;
static bool s_touchSessionInAdvancedZone = false;

// Dot position for UI (center when idle)
static int s_joyDotX = JOY_CX, s_joyDotY = JOY_CY;

// Slider drag state (for servo editor)
static bool s_sliderDragging = false;
static uint8_t s_activeSlider = 0xFF;
static int s_dragStartX = 0;

// Direction button positions (for UI highlighting)
static void getDirectionButtonPos(Direction dir, int* outX, int* outY) {
  if (dir == DIR_NONE) {
    *outX = JOY_CX;
    *outY = JOY_CY;
    return;
  }
  
  // Calculate button center position based on direction
  float angle = dir * 45.0f - 90.0f;  // 0=up, 45=upright, etc.
  float rad = angle * M_PI / 180.0f;
  *outX = JOY_CX + (int)(cosf(rad) * JOY_RADIUS);
  *outY = JOY_CY + (int)(sinf(rad) * JOY_RADIUS);
}

// Tank drive mixer: X/Y joystick → left/right speeds
// Y-axis (up/down) = forward/back
// X-axis (left/right) = turn
static void tankDriveMix(float joyX, float joyY, int8_t* outLeft, int8_t* outRight) {
  // Normalize to -1..+1
  float x = joyX / (float)JOY_RADIUS;   // Normal: right joystick = turn right
  float y = -joyY / (float)JOY_RADIUS;  // Invert Y (up = positive)
  
  if (x > 1.0f) x = 1.0f;
  if (x < -1.0f) x = -1.0f;
  if (y > 1.0f) y = 1.0f;
  if (y < -1.0f) y = -1.0f;
  
  // Tank drive mixing (swap left/right to fix steering)
  float left = y - x;   // Swapped: was y + x
  float right = y + x;  // Swapped: was y - x
  
  // Clamp to -1..+1
  if (left > 1.0f) left = 1.0f;
  if (left < -1.0f) left = -1.0f;
  if (right > 1.0f) right = 1.0f;
  if (right < -1.0f) right = -1.0f;
  
  *outLeft = (int8_t)(left * 100.0f);
  *outRight = (int8_t)(right * 100.0f);
}

static void processTouchInCircle(int cx, int cy, int px, int py,
                                 int8_t* outLeft, int8_t* outRight, int* outDotX, int* outDotY) {
  float dx = (float)(px - cx);
  float dy = (float)(py - cy);

  float distance = sqrtf(dx * dx + dy * dy);
  if (distance < 1.0f) distance = 1.0f;

  // Radial clamping
  int dotX = px, dotY = py;
  if (distance > JOY_RADIUS) {
    float scale = JOY_RADIUS / distance;
    dotX = cx + (int)(dx * scale);
    dotY = cy + (int)(dy * scale);
    dx *= scale;
    dy *= scale;
  }

  // Deadzone check
  if (distance < JOY_DEADZONE) {
    *outLeft = 0;
    *outRight = 0;
  } else {
    tankDriveMix(dx, dy, outLeft, outRight);
  }
  
  *outDotX = dotX;
  *outDotY = dotY;
}

static bool inCircle(int cx, int cy, int px, int py) {
  int dx = px - cx, dy = py - cy;
  return (dx * dx + dy * dy) <= (JOY_RADIUS * JOY_RADIUS);
}

void touchInit(void) {
  memset(&s_driveState, 0, sizeof(s_driveState));
  memset(&s_smoothedState, 0, sizeof(s_smoothedState));
  s_joyDotX = JOY_CX;
  s_joyDotY = JOY_CY;

  s_touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  s_ts.begin(s_touchSPI);
  s_ts.setRotation(1);
}

XPT2046_Touchscreen* touchGetTs(void) { return &s_ts; }
SPIClass* touchGetSPI(void) { return &s_touchSPI; }

void touchSetOverlayDismissed(void) {
  s_pressInQuickZone = false;
}

TouchZone touchUpdate(int page) {
  TouchZone zone = TOUCH_ZONE_NONE;
  int8_t rawLeft = 0, rawRight = 0;
  unsigned long now = millis();

  if (s_ts.tirqTouched() && s_ts.touched()) {
    TS_Point p = s_ts.getPoint();
    if (p.z > TOUCH_MIN_PRESSURE) {
      int screenX = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W - 1);
      int screenY = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H - 1);
      screenX = constrain(screenX, 0, SCREEN_W - 1);
      screenY = constrain(screenY, 0, SCREEN_H - 1);

      // DEBUG: Log ALL touches
      static unsigned long lastTouchLog = 0;
      if (now - lastTouchLog > 500) {  // Every 500ms
        Serial.printf("[Touch] X=%d Y=%d Page=%d\n", screenX, screenY, page);
        lastTouchLog = now;
      }

      s_lastInputMs = now;
      zone = touchGetZone(screenX, screenY, page);
      
      // Debug: Log navigation button touches
      if (zone == TOUCH_ZONE_NAV_SYSTEM || zone == TOUCH_ZONE_NAV_BEHAV || zone == TOUCH_ZONE_NAV_PROFILE) {
        Serial.printf("[Touch] Nav button at X=%d Y=%d Zone=%d\n", screenX, screenY, zone);
      }

      // Long-press quick action (bottom-right 2s)
      if (screenX >= QUICK_ZONE_X_MIN && screenY >= QUICK_ZONE_Y_MIN) {
        if (!s_pressInQuickZone) {
          s_pressInQuickZone = true;
          s_pressStartMs = now;
          s_quickActionFired = false;
        } else if (!s_quickActionFired && now - s_pressStartMs >= LONG_PRESS_MS) {
          zone = TOUCH_ZONE_QUICK_ACTION;
          s_quickActionFired = true;
        }
      } else {
        s_pressInQuickZone = false;
      }

      // Track for triple-tap (finger in zone during this press)
      if (screenX < ADVANCED_ZONE_X_MAX && screenY < ADVANCED_ZONE_Y_MAX) {
        s_touchSessionInAdvancedZone = true;
      }
      
      // Track animation button long-press on Behaviour page (500ms to toggle favorite)
      if (page == 1 && zone >= TOUCH_ZONE_ANIM_0 && zone <= TOUCH_ZONE_ANIM_5) {
        if (s_animPressZone != zone) {
          s_animPressZone = zone;
          s_animPressStartMs = now;
          s_animLongPressFired = false;
        } else if (!s_animLongPressFired && now - s_animPressStartMs >= 500) {
          zone = (TouchZone)(TOUCH_ZONE_ANIM_0 + 100 + (zone - TOUCH_ZONE_ANIM_0));
          s_animLongPressFired = true;
        }
      } else {
        s_animPressZone = -1;
      }

      // Only process buttons on Drive page (0)
      if (page == 0) {
        Direction dir = getDirectionFromTouch(screenX, screenY);
        s_currentDirection = dir;
        
        if (dir != DIR_NONE) {
          getSpeedsFromDirection(dir, &rawLeft, &rawRight);
          getDirectionButtonPos(dir, &s_joyDotX, &s_joyDotY);
        } else {
          s_joyDotX = JOY_CX;
          s_joyDotY = JOY_CY;
        }
        s_driveState.leftSpeed = rawLeft;
        s_driveState.rightSpeed = rawRight;
      } else {
        s_joyDotX = JOY_CX;
        s_joyDotY = JOY_CY;
      }
    }
  } else {
    // Release — count tap for triple-tap if finger was in top-left during press
    if (s_touchSessionInAdvancedZone) {
      if (now - s_tapWindowStartMs > TRIPLE_TAP_WINDOW_MS) {
        s_tapCount = 0;
        s_tapWindowStartMs = now;
      }
      s_tapCount++;
      if (s_tapCount >= 3) {
        zone = TOUCH_ZONE_ADVANCED;
        s_tapCount = 0;
      }
    }
    s_touchSessionInAdvancedZone = false;
    s_pressInQuickZone = false;
    s_quickActionFired = false;
    s_animPressZone = -1;
    s_animLongPressFired = false;
    // No touch — return to center
    s_joyDotX = JOY_CX;
    s_joyDotY = JOY_CY;
    s_driveState.leftSpeed = 0;
    s_driveState.rightSpeed = 0;
  }

  // Smooth interpolation: output = new*0.6 + prev*0.4
  s_smoothedState.leftSpeed  = (int8_t)((float)s_driveState.leftSpeed  * JOY_SMOOTH_FACTOR + (float)s_smoothedState.leftSpeed  * JOY_SMOOTH_PREV);
  s_smoothedState.rightSpeed = (int8_t)((float)s_driveState.rightSpeed * JOY_SMOOTH_FACTOR + (float)s_smoothedState.rightSpeed * JOY_SMOOTH_PREV);
  s_smoothedState.precisionMode = s_driveState.precisionMode;

  return zone;
}

void touchGetDriveState(DriveState* out) {
  if (out) *out = s_smoothedState;
}

void touchGetJoystickDots(int* joyX, int* joyY) {
  if (joyX) *joyX = s_joyDotX;
  if (joyY) *joyY = s_joyDotY;
}

unsigned long touchLastActivityMs(void) {
  return s_lastInputMs;
}

TouchZone touchGetZone(int screenX, int screenY, int page) {
  if (page == 0) {  // PAGE_DRIVE
    if (screenX >= CYD_LASER_FIRE_X && screenX < CYD_LASER_FIRE_X + CYD_LASER_FIRE_W &&
        screenY >= CYD_LASER_FIRE_Y && screenY < CYD_LASER_FIRE_Y + CYD_LASER_FIRE_H) {
      return TOUCH_ZONE_LASER_FIRE;
    }
    if (screenX >= CYD_LASER_PAD_X && screenX < CYD_LASER_PAD_X + CYD_LASER_PAD_W &&
        screenY >= CYD_LASER_PAD_Y && screenY < CYD_LASER_PAD_Y + CYD_LASER_PAD_H) {
      return TOUCH_ZONE_LASER_PAD;
    }
    // Single centered joystick zone
    if (inCircle(JOY_CX, JOY_CY, screenX, screenY)) return TOUCH_ZONE_LEFT_JOY;
    // Dock | Cancel | E-STOP — geometry from ui_draw.h (DRIVE_*)
    if (screenY >= DRIVE_BOTTOM_BTN_Y && screenY <= DRIVE_BOTTOM_BTN_Y + DRIVE_BOTTOM_BTN_H &&
        screenX >= DRIVE_DOCK_X && screenX < DRIVE_DOCK_X + DRIVE_DOCK_W) {
      return TOUCH_ZONE_DOCK_GO;
    }
    if (screenY >= DRIVE_BOTTOM_BTN_Y && screenY <= DRIVE_BOTTOM_BTN_Y + DRIVE_BOTTOM_BTN_H &&
        screenX >= DRIVE_CANCEL_X && screenX < DRIVE_CANCEL_X + DRIVE_CANCEL_W) {
      return TOUCH_ZONE_DOCK_CANCEL;
    }
    if (screenY >= DRIVE_BOTTOM_BTN_Y && screenY <= DRIVE_BOTTOM_BTN_Y + DRIVE_BOTTOM_BTN_H &&
        screenX >= DRIVE_ESTOP_X && screenX < DRIVE_ESTOP_X + DRIVE_ESTOP_W) {
      return TOUCH_ZONE_ESTOP;
    }
    // Four nav tiles: Sys | Beh | Prf | Aut
    if (screenY >= DRIVE_BOTTOM_BTN_Y && screenY <= DRIVE_BOTTOM_BTN_Y + DRIVE_BOTTOM_BTN_H &&
        screenX >= DRIVE_NAV_GRID_X && screenX < DRIVE_NAV_GRID_X + DRIVE_NAV_CELL_W) {
      return TOUCH_ZONE_NAV_SYSTEM;
    }
    if (screenY >= DRIVE_BOTTOM_BTN_Y && screenY <= DRIVE_BOTTOM_BTN_Y + DRIVE_BOTTOM_BTN_H &&
        screenX >= DRIVE_NAV_BEH_X && screenX < DRIVE_NAV_BEH_X + DRIVE_NAV_CELL_W) {
      return TOUCH_ZONE_NAV_BEHAV;
    }
    if (screenY >= DRIVE_BOTTOM_BTN_Y && screenY <= DRIVE_BOTTOM_BTN_Y + DRIVE_BOTTOM_BTN_H &&
        screenX >= DRIVE_NAV_PRF_X && screenX < DRIVE_NAV_PRF_X + DRIVE_NAV_CELL_W) {
      return TOUCH_ZONE_NAV_PROFILE;
    }
    if (screenY >= DRIVE_BOTTOM_BTN_Y && screenY <= DRIVE_BOTTOM_BTN_Y + DRIVE_BOTTOM_BTN_H &&
        screenX >= DRIVE_NAV_AUT_X && screenX < DRIVE_NAV_AUT_X + DRIVE_NAV_CELL_W) {
      return TOUCH_ZONE_NAV_AUTONOMY;
    }
    
    // Mood buttons on right side (physical joystick layout)
    const int cTop = uiContentTop();
    int midX = 320 / 2;  // 160
    for (int i = 0; i < 5; i++) {
      int bx = midX + 16 + (i % 2) * 72;
      int by = cTop + 30 + (i / 2) * 38;
      if (screenX >= bx && screenX < (bx + 64) &&
          screenY >= by && screenY < (by + 32)) {
        return (TouchZone)(TOUCH_ZONE_MOOD_CURIOUS + i);
      }
    }
  } else if (page == 1 || page == 2) {  // PAGE_BEHAVIOUR, PAGE_SYSTEM
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) return TOUCH_ZONE_NAV_BACK;
    
    // Behaviour page mood buttons (5 buttons in 2 columns)
    if (page == 1) {
      // NEW: 6 animation buttons in 3x2 grid
      const int behGridTop = uiContentTop() + 10;
      for (int i = 0; i < 6; i++) {
        int bx = 16 + (i % 3) * 100;
        int by = behGridTop + (i / 3) * 50;
        if (screenX >= bx && screenX < (bx + 90) &&
            screenY >= by && screenY < (by + 36)) {
          return (TouchZone)(TOUCH_ZONE_ANIM_0 + i);
        }
      }
    }
    
    // Profiles button on System page
    if (page == 2 && screenY >= 130 && screenY <= 162 && screenX >= 16 && screenX <= 116) {
      return TOUCH_ZONE_NAV_PROFILE;
    }
    // Servo Test button on System page
    if (page == 2 && screenY >= 130 && screenY <= 162 && screenX >= 130 && screenX <= 230) {
      return TOUCH_ZONE_NAV_SERVO_TEST;
    }
    // System row 2: Autonomy | Help | SD (see uiDrawPageSystem)
    if (page == 2 && screenY >= 168 && screenY <= 200 && screenX >= 4 && screenX <= 104) {
      return TOUCH_ZONE_NAV_AUTONOMY;
    }
    if (page == 2 && screenY >= 168 && screenY <= 200 && screenX >= 110 && screenX <= 210) {
      return TOUCH_ZONE_NAV_HELP;
    }
    if (page == 2 && screenY >= 168 && screenY <= 200 && screenX >= 216 && screenX <= 316) {
      return TOUCH_ZONE_NAV_SD;
    }
  } else if (page == 3) {  // PAGE_PROFILE
    // Profile cards: 3 cards centered
    const int cardW = 90;
    const int cardH = 120;
    const int cardSpacing = 10;
    const int startX = (320 - (cardW * 3 + cardSpacing * 2)) / 2;
    const int startY = uiContentTop() + 10;
    
    for (int i = 0; i < 3; i++) {
      int x = startX + i * (cardW + cardSpacing);
      int y = startY;
      
      // Edit button at bottom of card
      if (screenX >= (x + 5) && screenX < (x + cardW - 5) &&
          screenY >= (y + cardH - 24) && screenY < (y + cardH - 4)) {
        return (TouchZone)(TOUCH_ZONE_PROFILE_EDIT_0 + i);
      }
      
      // Profile card (excluding edit button area)
      if (screenX >= x && screenX < x + cardW && 
          screenY >= y && screenY < y + cardH - 24) {
        return (TouchZone)(TOUCH_ZONE_PROFILE_0 + i);
      }
    }
    
    // Back button
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) {
      return TOUCH_ZONE_NAV_BACK;
    }
  } else if (page == 4) {  // PAGE_SERVO_EDITOR
    // Slider zones (5 sliders)
    const int startY = uiContentTop() + 8;
    const int sliderH = 24;
    const int spacing = 28;
    const int sliderX = 120;
    const int sliderW = 180;
    
    for (int i = 0; i < 5; i++) {
      int y = startY + (i * spacing);
      if (screenX >= sliderX && screenX < (sliderX + sliderW) &&
          screenY >= (y - 2) && screenY < (y + sliderH)) {
        return (TouchZone)(TOUCH_ZONE_SLIDER_0 + i);
      }
    }
    
    // Save button (bottom left)
    if (screenY >= 204 && screenY <= 236 && screenX >= 20 && screenX <= 90) {
      return TOUCH_ZONE_SERVO_SAVE;
    }
    // Reset button (bottom center)
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 180) {
      return TOUCH_ZONE_SERVO_RESET;
    }
    // Back button (bottom right)
    if (screenY >= 204 && screenY <= 236 && screenX >= 230 && screenX <= 300) {
      return TOUCH_ZONE_NAV_BACK;
    }
  } else if (page == 5) {  // PAGE_SERVO_TEST
    // Servo sliders (2 columns, 5 rows each)
    const int startY = 40;
    const int sliderH = 16;
    const int spacing = 18;
    const int col1X = 10;
    const int col2X = 165;
    const int sliderX = 55;
    const int sliderW = 90;
    
    for (int i = 0; i < SERVO_COUNT; i++) {
      int colX = (i < 5) ? col1X : col2X;
      int row = (i < 5) ? i : (i - 5);
      int y = startY + (row * spacing);
      
      if (screenX >= (colX + sliderX) && screenX < (colX + sliderX + sliderW) &&
          screenY >= y && screenY < (y + sliderH)) {
        return (TouchZone)(TOUCH_ZONE_SERVO_SLIDER_0 + i);
      }
    }
    
    // Preset buttons
    if (screenY >= 204 && screenY <= 236) {
      if (screenX >= 5 && screenX <= 75) return TOUCH_ZONE_SERVO_SAVE_NEUTRAL;   // Save Neutral
      if (screenX >= 80 && screenX <= 135) return TOUCH_ZONE_SERVO_NEUTRAL;      // Neutral
      if (screenX >= 140 && screenX <= 185) return TOUCH_ZONE_SERVO_TEST1;       // Test1
      if (screenX >= 190 && screenX <= 235) return TOUCH_ZONE_SERVO_TEST2;       // Test2
      if (screenX >= 250 && screenX <= 310) return TOUCH_ZONE_NAV_BACK;          // Back
    }
  } else if (page == 6 || page == 7) {  // PAGE_AUTONOMY / PAGE_WAYPOINTS (same layout)
    const int c = uiContentTop();
    if (screenY >= c + 4 && screenY <= c + 26) {
      if (screenX < 160) return TOUCH_ZONE_AUTONOMY_TAB_LIVE;
      return TOUCH_ZONE_AUTONOMY_TAB_TUNE;
    }
    const int body = c + 30;
    const int rh = 16;
    if (g_autonomyUiTab == 0) {
      if (screenX >= 200 && screenX <= 310 && screenY >= body + 2 && screenY <= body + 30) {
        return TOUCH_ZONE_AUTONOMY_ARM;
      }
    } else {
      // Match ui_draw.cpp AU_TUNE_X_MINUS (84), AU_TUNE_BTN_W (36), AU_TUNE_X_PLUS (264); waypoint toggle 128+72
      const int auM0 = 84, auM1 = 120, auP0 = 264, auP1 = 300;
      int y = body + 8;
      if (screenY >= y && screenY <= y + rh) {
        if (screenX >= auM0 && screenX <= auM1) return TOUCH_ZONE_AUTONOMY_M_CLOSE;
        if (screenX >= auP0 && screenX <= auP1) return TOUCH_ZONE_AUTONOMY_P_CLOSE;
      }
      y += rh;
      if (screenY >= y && screenY <= y + rh) {
        if (screenX >= auM0 && screenX <= auM1) return TOUCH_ZONE_AUTONOMY_M_INT;
        if (screenX >= auP0 && screenX <= auP1) return TOUCH_ZONE_AUTONOMY_P_INT;
      }
      y += rh;
      if (screenY >= y && screenY <= y + rh) {
        if (screenX >= auM0 && screenX <= auM1) return TOUCH_ZONE_AUTONOMY_M_CUR;
        if (screenX >= auP0 && screenX <= auP1) return TOUCH_ZONE_AUTONOMY_P_CUR;
      }
      y += rh;
      if (screenY >= y && screenY <= y + rh) {
        if (screenX >= auM0 && screenX <= auM1) return TOUCH_ZONE_AUTONOMY_M_BRV;
        if (screenX >= auP0 && screenX <= auP1) return TOUCH_ZONE_AUTONOMY_P_BRV;
      }
      y += rh;
      if (screenY >= y && screenY <= y + rh && screenX >= 128 && screenX <= 200) {
        return TOUCH_ZONE_AUTONOMY_WAYPOINT;
      }
      y += rh + 4;
      if (screenY >= y && screenY <= y + rh) {
        if (screenX >= 52 && screenX < 114) return TOUCH_ZONE_AUTONOMY_PRESET_0;
        if (screenX >= 118 && screenX < 180) return TOUCH_ZONE_AUTONOMY_PRESET_1;
        if (screenX >= 184 && screenX < 246) return TOUCH_ZONE_AUTONOMY_PRESET_2;
        if (screenX >= 250 && screenX < 318) return TOUCH_ZONE_AUTONOMY_PRESET_3;
      }
    }
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) {
      return TOUCH_ZONE_NAV_BACK;
    }
  } else if (page == 8) {  // PAGE_HELP
    const int c = uiContentTop();
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) {
      return TOUCH_ZONE_NAV_BACK;
    }
    if (g_helpSection == 0) {
      for (int i = 0; i < 4; i++) {
        int y = c + 22 + i * 38;
        if (screenX >= 10 && screenX <= 310 && screenY >= y && screenY <= y + 32) {
          return (TouchZone)(TOUCH_ZONE_HELP_TOPIC_0 + i);
        }
      }
    }
  } else if (page == 9) {  // PAGE_SD_EXPLORER
    if (uiSdMemoryCoreRenameIsOpen()) {
      const int r1y = 92, r2y = 122, r3y = 152;
      const int bw = 94, gh = 26, g = 4;
      const int x0 = 14;
      const int x2 = (SCREEN_W - (2 * bw + g)) / 2;
      if (screenX >= x0 && screenX < x0 + bw && screenY >= r1y && screenY < r1y + gh) {
        return TOUCH_ZONE_SD_RENAME_CH_DEC;
      }
      if (screenX >= x0 + bw + g && screenX < x0 + 2 * bw + g && screenY >= r1y && screenY < r1y + gh) {
        return TOUCH_ZONE_SD_RENAME_CH_INC;
      }
      if (screenX >= x0 + 2 * (bw + g) && screenX < x0 + 3 * bw + 2 * g && screenY >= r1y && screenY < r1y + gh) {
        return TOUCH_ZONE_SD_RENAME_BKSP;
      }
      if (screenX >= x2 && screenX < x2 + bw && screenY >= r2y && screenY < r2y + gh) {
        return TOUCH_ZONE_SD_RENAME_CUR_L;
      }
      if (screenX >= x2 + bw + g && screenX < x2 + 2 * bw + g && screenY >= r2y && screenY < r2y + gh) {
        return TOUCH_ZONE_SD_RENAME_CUR_R;
      }
      if (screenX >= x2 && screenX < x2 + bw && screenY >= r3y && screenY < r3y + gh) {
        return TOUCH_ZONE_SD_RENAME_CANCEL;
      }
      if (screenX >= x2 + bw + g && screenX < x2 + 2 * bw + g && screenY >= r3y && screenY < r3y + gh) {
        return TOUCH_ZONE_SD_RENAME_OK;
      }
      return TOUCH_ZONE_NONE;
    }
    if (uiSdExplorerConfirmIsOpen()) {
      if (screenY >= 120 && screenY <= 156) {
        if (screenX >= 40 && screenX <= 140) return TOUCH_ZONE_SD_CONFIRM_YES;
        if (screenX >= 180 && screenX <= 280) return TOUCH_ZONE_SD_CONFIRM_NO;
      }
      return TOUCH_ZONE_NONE;
    }
    if (uiSdExplorerPreviewIsOpen()) {
      if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) {
        return TOUCH_ZONE_NAV_BACK;
      }
      return TOUCH_ZONE_NONE;
    }
    const int c = uiContentTop();
    /* Memory Core shortcuts (6 tiles) */
    {
      const int n = 6;
      const int gap = 2;
      const int totalW = 320 - 8;
      const int btnW = (totalW - (n - 1) * gap) / n;
      int sy0 = c + UI_MC_SHORTCUT_Y;
      if (screenY >= sy0 && screenY < sy0 + UI_MC_SHORTCUT_H) {
        for (int b = 0; b < n; b++) {
          int bx = 4 + b * (btnW + gap);
          if (screenX >= bx && screenX < bx + btnW) {
            return (TouchZone)(TOUCH_ZONE_MC_SC_0 + b);
          }
        }
      }
    }
    const int listTop = c + UI_MC_LIST_OFF;
    for (int r = 0; r < UI_MC_LIST_ROWS; r++) {
      int ry0 = listTop + r * UI_MC_ROW_H;
      if (screenX >= 4 && screenX <= 304 && screenY >= ry0 && screenY < ry0 + UI_MC_ROW_H) {
        return (TouchZone)(TOUCH_ZONE_SD_LIST_0 + r);
      }
    }
    const int tbY = c + UI_MC_LIST_OFF + UI_MC_LIST_ROWS * UI_MC_ROW_H + 4;
    const int nTb = 7;
    const int gapTb = 2;
    const int totalWTb = 320 - 8;
    const int btnW = (totalWTb - (nTb - 1) * gapTb) / nTb;
    for (int b = 0; b < nTb; b++) {
      int bx = 4 + b * (btnW + gapTb);
      if (screenY >= tbY && screenY <= tbY + 22 && screenX >= bx && screenX < bx + btnW) {
        if (b == 0) return TOUCH_ZONE_SD_UP;
        if (b == 1) return TOUCH_ZONE_SD_OPEN;
        if (b == 2) return TOUCH_ZONE_SD_REFRESH;
        if (b == 3) return TOUCH_ZONE_SD_PG_PREV;
        if (b == 4) return TOUCH_ZONE_SD_PG_NEXT;
        if (b == 5) return TOUCH_ZONE_SD_DELETE;
        return TOUCH_ZONE_SD_RENAME;
      }
    }
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) {
      return TOUCH_ZONE_NAV_BACK;
    }
  }
  return TOUCH_ZONE_NONE;
}
