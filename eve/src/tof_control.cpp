#include "tof_control.h"
#include "config.h"

void tofInit(void) {
#if EVE_ENABLE_TOF
  Serial.println(F("[EVE][ToF] init"));
#else
  Serial.println(F("[EVE][ToF] disabled"));
#endif
}

void tofTick(void) {
}
