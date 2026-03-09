/*******************************************************************************
 * dock_neopixel.cpp
 * Non-blocking NeoPixel patterns (millis-based, no delay)
 ******************************************************************************/

#include "dock_neopixel.h"
#include "dock_config.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

static Adafruit_NeoPixel strip(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
static uint8_t g_brightness = NEOPIXEL_BRIGHTNESS_DEFAULT;
static uint32_t g_phase_start = 0;
static NeoPixelState g_last_state = NP_STATE_FAULT;  /* force first update */

void dockNeoPixelBegin(void) {
  strip.begin();
  strip.setBrightness(g_brightness);
  strip.clear();
  strip.show();
  g_phase_start = millis();
}

void dockNeoPixelSetBrightness(uint8_t b) {
  g_brightness = b;
}

static uint8_t breathe(uint32_t elapsed, uint32_t period_ms) {
  uint32_t half = period_ms / 2;
  uint32_t phase = elapsed % period_ms;
  if (phase < half) {
    return map(phase, 0, half, 10, 200);
  } else {
    return map(phase, half, period_ms, 200, 10);
  }
}

void dockNeoPixelUpdate(NeoPixelState state, bool mouth_blocked_warn) {
  uint32_t now = millis();
  uint32_t elapsed = now - g_phase_start;

  /* Reset phase on state change */
  if (state != g_last_state) {
    g_phase_start = now;
    elapsed = 0;
    g_last_state = state;
  }

  uint8_t b = g_brightness;
  uint32_t color = 0;
  bool strobe = false;

  switch (state) {
    case NP_STATE_NOT_DOCKED: {
      /* Soft breathing blue */
      uint8_t breath = breathe(elapsed, NEOPIXEL_BREATHE_PERIOD_MS);
      strip.setBrightness(breath);
      strip.fill(strip.Color(0, 0, 255));
      break;
    }
    case NP_STATE_DOCKED_IDLE: {
      /* Solid purple (or yellow) */
      strip.setBrightness(b);
      strip.fill(strip.Color(180, 0, 255));  /* purple */
      break;
    }
    case NP_STATE_CHARGING: {
      /* Chase animation in orange */
      uint32_t phase = elapsed % NEOPIXEL_CHASE_PERIOD_MS;
      int pos = (phase * NEOPIXEL_COUNT) / NEOPIXEL_CHASE_PERIOD_MS;
      strip.setBrightness(b);
      strip.clear();
      for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        int dist = abs((int)i - pos);
        if (dist <= 1 || (pos >= NEOPIXEL_COUNT - 1 && i == 0)) {
          strip.setPixelColor(i, strip.Color(255, 100, 0));
        } else if (dist == 2) {
          strip.setPixelColor(i, strip.Color(80, 30, 0));
        }
      }
      break;
    }
    case NP_STATE_CHARGED: {
      /* Solid green with gentle pulse */
      uint8_t pulse = 150 + (breathe(elapsed, NEOPIXEL_CHARGED_PULSE_MS) / 4);
      strip.setBrightness(constrain(pulse, 80, 255));
      strip.fill(strip.Color(0, 255, 0));
      break;
    }
    case NP_STATE_FAULT: {
      /* Fast red flash */
      uint32_t bl = elapsed % (NEOPIXEL_FAULT_BLINK_MS * 2);
      strobe = (bl < NEOPIXEL_FAULT_BLINK_MS);
      strip.setBrightness(200);
      strip.fill(strobe ? strip.Color(255, 0, 0) : 0);
      break;
    }
    default:
      strip.clear();
      break;
  }

  /* Mouth blocked overlay: brief red blink every second */
  if (mouth_blocked_warn && state != NP_STATE_FAULT) {
    uint32_t sec = elapsed / 1000;
    uint32_t msec = elapsed % 1000;
    if (msec < 100) {
      strip.setBrightness(200);
      strip.fill(strip.Color(255, 0, 0));
    }
  }

  strip.show();
}
