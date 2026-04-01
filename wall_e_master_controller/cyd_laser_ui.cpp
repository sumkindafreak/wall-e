// ============================================================
//  CYD laser UI — drag aim + armed flag
// ============================================================

#include "cyd_laser_ui.h"
#include "motion_engine.h"
#include <Arduino.h>

static bool     s_armed = false;
static bool     s_dragging = false;
static uint8_t  s_aimPan = 50;
static uint8_t  s_aimTilt = 50;

void cydLaserUiInit(void) {
  s_armed = false;
  s_dragging = false;
  s_aimPan = s_aimTilt = 50;
  Serial.println(F("[LaserUI] Init (armed=OFF)"));
}

void cydLaserUiSetArmed(bool on) {
  s_armed = on;
  if (!on) {
    /* Safety: disarm clears drag */
    s_dragging = false;
  }
}

bool cydLaserUiGetArmed(void) {
  return s_armed;
}

void cydLaserUiToggleArmed(void) {
  s_armed = !s_armed;
  if (!s_armed) s_dragging = false;
  Serial.printf("[LaserUI] Armed=%d\n", s_armed ? 1 : 0);
}

void cydLaserUiBeginFrame(void) {
  /* Drag ends when touch layer stops calling DragFromScreen */
}

void cydLaserUiCancelDrag(void) {
  s_dragging = false;
  motionSetJoystickOverride(SERVO_HEAD_PAN, 0.0f, 0.0f);
  motionSetJoystickOverride(SERVO_HEAD_TILT, 0.0f, 0.0f);
}

void cydLaserUiDragFromScreen(int screenX, int screenY) {
  int x = screenX - CYD_LASER_PAD_X;
  int y = screenY - CYD_LASER_PAD_Y;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x > CYD_LASER_PAD_W - 1) x = CYD_LASER_PAD_W - 1;
  if (y > CYD_LASER_PAD_H - 1) y = CYD_LASER_PAD_H - 1;

  s_aimPan = (uint8_t)((x * 100) / (CYD_LASER_PAD_W - 1));
  s_aimTilt = (uint8_t)((y * 100) / (CYD_LASER_PAD_H - 1));
  s_dragging = true;
}

bool cydLaserUiIsDraggingHead(void) {
  return s_dragging;
}

void cydLaserUiApplyMotion(void) {
  if (!s_dragging) return;
  float panDeg = 30.0f + (s_aimPan / 100.0f) * 120.0f;
  float tiltDeg = 30.0f + (s_aimTilt / 100.0f) * 120.0f;
  float offPan = panDeg - 90.0f;
  float offTilt = tiltDeg - 90.0f;
  motionSetJoystickOverride(SERVO_HEAD_PAN, offPan, 1.0f);
  motionSetJoystickOverride(SERVO_HEAD_TILT, offTilt, 1.0f);
}

uint16_t cydLaserUiGetExtraFlags(void) {
  return s_armed ? FLAG_LASER : 0;
}

void cydLaserUiGetAim(uint8_t* panOut, uint8_t* tiltOut) {
  if (panOut) *panOut = s_aimPan;
  if (tiltOut) *tiltOut = s_aimTilt;
}
