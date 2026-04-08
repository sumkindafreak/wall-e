#include "servo_control.h"
#include "config.h"

void servoInit(void) {
#if EVE_ENABLE_SERVOS
  Serial.println(F("[EVE][SERVO] init"));
#else
  Serial.println(F("[EVE][SERVO] disabled"));
#endif
}

void servoTick(void) {
}

void servoSetAngles(int16_t leftDeg, int16_t rightDeg) {
  (void)leftDeg;
  (void)rightDeg;
#if EVE_ENABLE_SERVOS
#endif
}
