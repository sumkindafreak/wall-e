// ============================================================
//  CYD laser UI — armed flag only (beam follows head on base)
// ============================================================

#include "cyd_laser_ui.h"
#include "motion_engine.h"
#include <Arduino.h>

static bool s_armed = false;

void cydLaserUiInit(void) {
  s_armed = false;
  motionSetJoystickOverride(SERVO_HEAD_PAN, 0.0f, 0.0f);
  motionSetJoystickOverride(SERVO_HEAD_TILT, 0.0f, 0.0f);
  Serial.println(F("[LaserUI] Init (beam=OFF)"));
}

void cydLaserUiSetArmed(bool on) {
  s_armed = on;
  if (!on) {
    motionSetJoystickOverride(SERVO_HEAD_PAN, 0.0f, 0.0f);
    motionSetJoystickOverride(SERVO_HEAD_TILT, 0.0f, 0.0f);
  }
}

bool cydLaserUiGetArmed(void) {
  return s_armed;
}

void cydLaserUiToggleArmed(void) {
  s_armed = !s_armed;
  if (!s_armed) {
    motionSetJoystickOverride(SERVO_HEAD_PAN, 0.0f, 0.0f);
    motionSetJoystickOverride(SERVO_HEAD_TILT, 0.0f, 0.0f);
  }
  Serial.printf("[LaserUI] Beam=%d\n", s_armed ? 1 : 0);
}

uint16_t cydLaserUiGetExtraFlags(void) {
  return s_armed ? FLAG_LASER : 0;
}
