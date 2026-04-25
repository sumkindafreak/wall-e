#include "servo_control.h"
#include "config.h"

static int16_t s_headPanTarget = 90;
static int16_t s_headPanCurrent = 90;
static int16_t s_rightArmTarget = 90;

void servoInit(void) {
#if EVE_ENABLE_SERVOS
  Serial.println(F("[EVE][SERVO] init"));
#else
  Serial.println(F("[EVE][SERVO] disabled"));
#endif
  s_headPanTarget = s_headPanCurrent = 90;
  s_rightArmTarget = 90;
}

void servoSetHeadPanTarget(int16_t deg) {
  s_headPanTarget = (int16_t)constrain((int)deg, 45, 135);
}

void servoSetRightArmTarget(int16_t deg) {
  s_rightArmTarget = (int16_t)constrain((int)deg, 0, 180);
}

void servoTick(void) {
#if EVE_ENABLE_SERVOS
  int32_t d = (int32_t)s_headPanTarget - (int32_t)s_headPanCurrent;
  int32_t step = d / 7;
  if (step == 0 && d != 0) {
    step = (d > 0) ? 1 : -1;
  }
  s_headPanCurrent = (int16_t)((int32_t)s_headPanCurrent + step);
#endif
}

void servoSetAngles(int16_t leftDeg, int16_t rightDeg) {
  (void)rightDeg;
#if EVE_ENABLE_SERVOS
  s_headPanTarget = (int16_t)constrain((int)leftDeg, 45, 135);
  s_headPanCurrent = s_headPanTarget;
#else
  (void)leftDeg;
#endif
}
