// ============================================================
//  WALL-E Master Controller — Touch Input
//  Proper virtual joystick math: deadzone, radial clamping, smoothing
// ============================================================

#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include "protocol.h"
#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// ------------------------------------------------------------
//  Touch zones (for page nav)
// ------------------------------------------------------------
typedef enum {
  TOUCH_ZONE_NONE,
  TOUCH_ZONE_LEFT_JOY,
  TOUCH_ZONE_RIGHT_JOY,
  TOUCH_ZONE_ESTOP,
  TOUCH_ZONE_NAV_BEHAV,
  TOUCH_ZONE_NAV_SYSTEM,
  TOUCH_ZONE_NAV_BACK,
  TOUCH_ZONE_QUICK_ACTION,  // Long-press 2s bottom-right
  TOUCH_ZONE_ADVANCED       // Triple-tap top-left
} TouchZone;

#define LONG_PRESS_MS  2000
#define TRIPLE_TAP_WINDOW_MS  800

// Page indices (0=Drive, 1=Behaviour, 2=System — pass as int)
// ------------------------------------------------------------
//  Touch config (CYD XPT2046)
// ------------------------------------------------------------
#define TOUCH_MIN_PRESSURE 200
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 240
#define TOUCH_Y_MAX 3800

// ------------------------------------------------------------
//  Joystick math
// ------------------------------------------------------------
#define JOY_DEADZONE_PCT  10    // 8-12% radial deadzone
#define JOY_SMOOTH_FACTOR 0.6f  // output = new*0.6 + prev*0.4

// ------------------------------------------------------------
//  API
// ------------------------------------------------------------
void touchInit(void);
TouchZone touchUpdate(int page);           // Call each loop — reads touch, updates drive state, returns zone
void touchSetOverlayDismissed(void);       // Clear long-press after overlay closed (optional)

void touchGetDriveState(DriveState* out);  // Smoothed output
void touchGetJoystickDots(int* joyX, int* joyY);  // For UI dot
unsigned long touchLastActivityMs(void);   // For safety lock (200ms → STOP)

TouchZone touchGetZone(int screenX, int screenY, int page);  // page: 0=Drive, 1=Behaviour, 2=System

XPT2046_Touchscreen* touchGetTs(void);
SPIClass* touchGetSPI(void);

#endif // TOUCH_INPUT_H
