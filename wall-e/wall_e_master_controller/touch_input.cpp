// ============================================================
//  WALL-E Master Controller — Touch Input Implementation
//  Proper joystick math: deadzone, radial clamping, smoothing
// ============================================================

#include "touch_input.h"
#include "ui_buttons.h"
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <Arduino.h>
#include <math.h>

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

static SPIClass s_touchSPI(VSPI);
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

// Triple-tap top-left (count on release when finger was in zone during press)
static int s_tapCount = 0;
static unsigned long s_tapWindowStartMs = 0;
static bool s_touchSessionInAdvancedZone = false;

// Dot position for UI (center when idle)
static int s_joyDotX = JOY_CX, s_joyDotY = JOY_CY;

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

      s_lastInputMs = now;
      zone = touchGetZone(screenX, screenY, page);

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
  } else if (page == 1 || page == 2) {  // PAGE_BEHAVIOUR, PAGE_SYSTEM
    if (screenY >= 204 && screenY <= 236 && screenX >= 110 && screenX <= 210) return TOUCH_ZONE_NAV_BACK;
  }
  return TOUCH_ZONE_NONE;
}
