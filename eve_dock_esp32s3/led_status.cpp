#include "led_status.h"
#include <Adafruit_NeoPixel.h>
static Adafruit_NeoPixel s_px(DOCK_NEO_COUNT, DOCK_PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
static LedStatusMode s_mode = LED_MODE_OFF;
#define LED_MAX_BRIGHT 48u
void led_init(void) { s_px.begin(); s_px.setBrightness(255); s_px.clear(); s_px.show(); }
void led_set_mode(LedStatusMode m) { s_mode = m; }
LedStatusMode led_get_mode(void) { return s_mode; }
void led_event_button_feedback(void) { s_mode = LED_MODE_PURPLE_FLASH; }
static uint8_t pulse_(uint32_t now_ms, uint32_t period, uint8_t maxB) { uint32_t ph = now_ms % period; uint32_t half = period / 2u; uint32_t b = ph < half ? ph * maxB / half : (period - ph) * maxB / half; return (uint8_t)(b > maxB ? maxB : b); }
void led_update(uint32_t now_ms) {
  switch (s_mode) {
    case LED_MODE_OFF: s_px.setPixelColor(0, 0, 0, 0); break;
    case LED_MODE_BLUE_PULSE: s_px.setPixelColor(0, 0, 0, pulse_(now_ms, 2000u, LED_MAX_BRIGHT)); break;
    case LED_MODE_STANDBY: s_px.setPixelColor(0, 0, 0, pulse_(now_ms, 5000u, 24u)); break;
    case LED_MODE_GREEN_SOLID: s_px.setPixelColor(0, 0, LED_MAX_BRIGHT, 0); break;
    case LED_MODE_PURPLE_FLASH: s_px.setPixelColor(0, 32, 0, 48); break;
    case LED_MODE_RED_BLINK: s_px.setPixelColor(0, ((now_ms / 200u) & 1u) ? LED_MAX_BRIGHT : 0, 0, 0); break;
  }
  s_px.show();
}
