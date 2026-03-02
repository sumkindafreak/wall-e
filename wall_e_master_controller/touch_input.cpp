// ============================================================
//  WALL-E Master Controller — Touch Input Implementation
//  Proper joystick math: deadzone, radial clamping, smoothing
// ============================================================

#include "touch_input.h"
#include "ui_buttons.h"
#include "motion_engine.h"  // For SERVO_COUNT
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Arduino.h>
#include <math.h>

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

#if CONFIG_IDF_TARGET_ESP32S3
static SPIClass s_touchSPI(HSPI);  // ESP32-S3 has HSPI, no VSPI
#else
static SPIClass s_touchSPI(VSPI);
#endif
static XPT2046_Touchscreen s_ts(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_W 320
#define SCREEN_H 240
#define QUICK_ZONE_X_MIN 270
#define QUICK_ZONE_Y_MIN 210
#define ADVANCED_ZONE_X_MAX 50
#define ADVANCED_ZONE_Y_MAX 50

// Single centered joystick
#define JOY_CX       160  // Center X
#define JOY_CY       126  // Center Y
#define JOY_RADIUS   70   // Larger radius
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
          // Long press detected - toggle favorite
          zone = (TouchZone)(TOUCH_ZONE_ANIM_0 + 100 + (zone - TOUCH_ZONE_ANIM_0));  // Special zone for "long press"
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
    // Single centered joystick zone
    if (inCircle(JOY_CX, JOY_CY, screenX, screenY)) return TOUCH_ZONE_LEFT_JOY;
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) return TOUCH_ZONE_ESTOP;
    if (screenY >= 208 && screenY <= 232 && screenX >= 250 && screenX <= 310) return TOUCH_ZONE_NAV_BEHAV;
    if (screenY >= 208 && screenY <= 232 && screenX >= 182 && screenX <= 242) return TOUCH_ZONE_NAV_SYSTEM;
    
    // Physical joystick mode: buttons on left side under "Battery"
    // cTop = 30 (TOP_BAR_HEIGHT) + 22 (TELEM_STRIP_H) = 52
    const int cTop = 52;
    // System button (top) - positioned at cTop + 30
    if (screenY >= (cTop + 30) && screenY <= (cTop + 62) && screenX >= 8 && screenX <= 88) return TOUCH_ZONE_NAV_SYSTEM;
    // Behaviour button (below) - positioned at cTop + 68
    if (screenY >= (cTop + 68) && screenY <= (cTop + 100) && screenX >= 8 && screenX <= 88) return TOUCH_ZONE_NAV_BEHAV;
    
    // Mood buttons on right side (physical joystick layout)
    int midX = 320 / 2;  // 160
    for (int i = 0; i < 5; i++) {
      int bx = midX + 16 + (i % 2) * 72;  // 176 or 248
      int by = cTop + 30 + (i / 2) * 45;   // 82, 127, 172
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
      for (int i = 0; i < 6; i++) {
        int bx = 16 + (i % 3) * 100;
        int by = 50 + (i / 3) * 50;
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
    // NEW: Autonomy button on System page (16, 168, 100x32)
    if (page == 2 && screenY >= 168 && screenY <= 200 && screenX >= 16 && screenX <= 116) {
      return TOUCH_ZONE_NAV_AUTONOMY;
    }
  } else if (page == 3) {  // PAGE_PROFILE
    // Profile cards: 3 cards centered
    const int cardW = 90;
    const int cardH = 120;
    const int cardSpacing = 10;
    const int startX = (320 - (cardW * 3 + cardSpacing * 2)) / 2;
    const int startY = 50;
    
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
    const int startY = 45;
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
  } else if (page == 6) {  // PAGE_AUTONOMY
    // Toggle button (200, 50, 100x32)
    if (screenX >= 200 && screenX <= 300 && screenY >= 50 && screenY <= 82) {
      return TOUCH_ZONE_AUTONOMY_TOGGLE;
    }
    // Back button
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) {
      return TOUCH_ZONE_NAV_BACK;
    }
  }
  return TOUCH_ZONE_NONE;
}
