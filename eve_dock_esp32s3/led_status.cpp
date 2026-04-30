#include "led_status.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel s_px(DOCK_NEO_COUNT, DOCK_PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#if DOCK_ONBOARD_RGB_HEARTBEAT && (DOCK_PIN_ONBOARD_RGB >= 0)
static Adafruit_NeoPixel s_onboard(1, DOCK_PIN_ONBOARD_RGB, NEO_GRB + NEO_KHZ800);
#endif
static LedStatusMode s_mode = LED_MODE_OFF;

#define LED_STATUS_BRIGHT 48u
#define LED_AURA_BRIGHT 28u
#define LED_STATUS_IDX 0u
#define LED_AURA_IDX 1u

void led_init(void) {
  s_px.begin();
  s_px.setBrightness(255);
  s_px.clear();
  s_px.show();
#if DOCK_ONBOARD_RGB_HEARTBEAT && (DOCK_PIN_ONBOARD_RGB >= 0)
  s_onboard.begin();
  s_onboard.setBrightness(32);
  s_onboard.clear();
  s_onboard.show();
#endif
}

void led_set_mode(LedStatusMode m) {
  s_mode = m;
}

LedStatusMode led_get_mode(void) {
  return s_mode;
}

void led_event_button_feedback(void) {
  s_mode = LED_MODE_PURPLE_FLASH;
}

static uint8_t pulse_(uint32_t now_ms, uint32_t period, uint8_t maxB) {
  uint32_t ph = now_ms % period;
  uint32_t half = period / 2u;
  uint32_t b = ph < half ? ph * maxB / half : (period - ph) * maxB / half;
  return (uint8_t)(b > maxB ? maxB : b);
}

static void setPixel_(uint16_t idx, uint8_t r, uint8_t g, uint8_t b) {
  if (idx < DOCK_NEO_COUNT) {
    s_px.setPixelColor(idx, r, g, b);
  }
}

void led_update(uint32_t now_ms) {
#if DOCK_ONBOARD_RGB_HEARTBEAT && (DOCK_PIN_ONBOARD_RGB >= 0)
  uint8_t hb = pulse_(now_ms, 1800u, 28u);
  if (s_mode == LED_MODE_GREEN_SOLID) {
    s_onboard.setPixelColor(0, 0, hb, 0);
  } else if (s_mode == LED_MODE_BLUE_PULSE || s_mode == LED_MODE_STANDBY) {
    s_onboard.setPixelColor(0, 0, 0, hb);
  } else if (s_mode == LED_MODE_PURPLE_FLASH) {
    s_onboard.setPixelColor(0, (uint8_t)(hb / 2u), 0, hb);
  } else {
    s_onboard.setPixelColor(0, hb, 0, 0);
  }
  s_onboard.show();
#endif

  s_px.clear();

  switch (s_mode) {
    case LED_MODE_OFF:
      break;

    case LED_MODE_BLUE_PULSE: {
      uint8_t statusB = pulse_(now_ms, 2000u, LED_STATUS_BRIGHT);
      uint8_t auraB = pulse_(now_ms + 500u, 3200u, LED_AURA_BRIGHT);
      setPixel_(LED_STATUS_IDX, 0, 0, statusB);
      setPixel_(LED_AURA_IDX, 0, 0, auraB);
      break;
    }

    case LED_MODE_STANDBY: {
      uint8_t b = pulse_(now_ms, 5000u, 18u);
      setPixel_(LED_STATUS_IDX, 0, 0, b);
      setPixel_(LED_AURA_IDX, 0, 0, (uint8_t)(b / 2u));
      break;
    }

    case LED_MODE_GREEN_SOLID: {
      uint8_t aura = pulse_(now_ms, 2600u, LED_AURA_BRIGHT);
      setPixel_(LED_STATUS_IDX, 0, LED_STATUS_BRIGHT, 0);
      setPixel_(LED_AURA_IDX, 0, aura, (uint8_t)(aura / 2u));
      break;
    }

    case LED_MODE_PURPLE_FLASH: {
      uint8_t on = ((now_ms / 120u) & 1u) ? 48u : 0u;
      setPixel_(LED_STATUS_IDX, (uint8_t)(on / 2u), 0, on);
      setPixel_(LED_AURA_IDX, (uint8_t)(on / 2u), 0, on);
      break;
    }

    case LED_MODE_RED_BLINK: {
      uint8_t on = ((now_ms / 200u) & 1u) ? LED_STATUS_BRIGHT : 0u;
      setPixel_(LED_STATUS_IDX, on, 0, 0);
      setPixel_(LED_AURA_IDX, (uint8_t)(on / 2u), 0, 0);
      break;
    }
  }

  s_px.show();
}
