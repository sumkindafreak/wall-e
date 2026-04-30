#include "neopixel_control.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>

#if EVE_ONBOARD_RGB_HEARTBEAT && (EVE_ONBOARD_RGB_PIN >= 0)
static Adafruit_NeoPixel s_onboard(1, EVE_ONBOARD_RGB_PIN, NEO_GRB + NEO_KHZ800);
#endif

static uint8_t s_pattern = 0;

static uint8_t pulse_(uint32_t nowMs, uint32_t periodMs, uint8_t maxB) {
  uint32_t ph = nowMs % periodMs;
  uint32_t half = periodMs / 2u;
  uint32_t b = ph < half ? ph * maxB / half : (periodMs - ph) * maxB / half;
  return (uint8_t)(b > maxB ? maxB : b);
}

void neopixelInit(void) {
#if EVE_ONBOARD_RGB_HEARTBEAT && (EVE_ONBOARD_RGB_PIN >= 0)
  s_onboard.begin();
  s_onboard.setBrightness(255);
  s_onboard.clear();
  s_onboard.show();
  Serial.print(F("[EVE][RGB] onboard heartbeat pin="));
  Serial.println(EVE_ONBOARD_RGB_PIN);
#endif
#if EVE_ENABLE_NEOPIXEL
  Serial.println(F("[EVE][NeoPixel] init"));
#else
  Serial.println(F("[EVE][NeoPixel] disabled"));
#endif
}

void neopixelTick(void) {
#if EVE_ONBOARD_RGB_HEARTBEAT && (EVE_ONBOARD_RGB_PIN >= 0)
  uint8_t hb = pulse_(millis(), 1800u, EVE_ONBOARD_RGB_BRIGHTNESS);
  switch (s_pattern) {
    case 4:
      s_onboard.setPixelColor(0, 0, hb, 0);
      break;
    case 5:
      s_onboard.setPixelColor(0, (uint8_t)(hb / 2u), 0, hb);
      break;
    case 3:
      s_onboard.setPixelColor(0, hb, 0, 0);
      break;
    default:
      s_onboard.setPixelColor(0, 0, 0, hb);
      break;
  }
  s_onboard.show();
#endif
}

void neopixelSetPattern(uint8_t pattern) {
  s_pattern = pattern;
#if EVE_ENABLE_NEOPIXEL
#endif
}
