#include "tof_control.h"
#include "config.h"
#include "eve_spatial_awareness.h"

void tofInit(void) {
#if EVE_ENABLE_TOF
  Serial.println(F("[EVE][ToF] spatial awareness active (see eve_spatial_awareness)"));
#else
  Serial.println(F("[EVE][ToF] disabled"));
#endif
}

void tofTick(void) {
#if EVE_ENABLE_TOF
  eveSpatialAwarenessTick(millis());
#endif
}
