#include "eyes_control.h"
#include "config.h"

void eyesInit(void) {
#if EVE_ENABLE_EYES
  Serial.println(F("[EVE][EYES] init (enable in config when TFT wired)"));
#else
  Serial.println(F("[EVE][EYES] disabled — set EVE_ENABLE_EYES and pins in config.h"));
#endif
}

void eyesTick(void) {
}

void eyesSetMode(uint8_t mode) {
  (void)mode;
#if EVE_ENABLE_EYES
#endif
}
