#include "neopixel_control.h"
#include "config.h"

void neopixelInit(void) {
#if EVE_ENABLE_NEOPIXEL
  Serial.println(F("[EVE][NeoPixel] init"));
#else
  Serial.println(F("[EVE][NeoPixel] disabled"));
#endif
}

void neopixelTick(void) {
}

void neopixelSetPattern(uint8_t pattern) {
  (void)pattern;
#if EVE_ENABLE_NEOPIXEL
#endif
}
