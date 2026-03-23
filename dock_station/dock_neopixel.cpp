/*******************************************************************************
 * dock_neopixel.cpp
 * Simple FastLED-based status strip on PIN_STATUS_NEOPIXEL (GPIO2)
 *
 * Design (11 LEDs total: 0-9 strip, 10 = status):
 *   NOT_DOCKED   : dim amber breathing bar on 0-6, status LED green
 *   DOCKED_IDLE  : 0-3 green (dock present), 4-6 amber, status LED orange if mouth blocked else green
 *   CHARGING     : 0-3 green, 4-9 orange bar scaled by current, status LED green
 *   CHARGED      : 0-9 green with slow pulse, status LED green
 *   FAULT        : full strip red blink, status LED red
 *   CALLOUT      : amber chase along full strip, status LED orange
 ******************************************************************************/

#include "dock_neopixel.h"
#include "dock_config.h"
#include "dock_state.h"
#include <FastLED.h>
#include <Arduino.h>

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

static CRGB leds[NEOPIXEL_COUNT];
static uint8_t g_brightness = NEOPIXEL_BRIGHTNESS_DEFAULT;
static uint32_t g_phase_start = 0;
static NeoPixelState g_last_state = NP_STATE_NOT_DOCKED;
static DockFaultCode g_last_fault = FAULT_NONE;
static uint8_t g_attention_flash_count = 0;
static bool g_attention_last_on = false;

static uint8_t breathe(uint32_t elapsed, uint32_t period_ms, uint8_t minB, uint8_t maxB) {
  uint32_t half = period_ms / 2;
  uint32_t phase = elapsed % period_ms;
  if (phase < half)
    return map(phase, 0, half, minB, maxB);
  return map(phase, half, period_ms, maxB, minB);
}

void dockNeoPixelBegin(void) {
  FastLED.addLeds<LED_TYPE, PIN_STATUS_NEOPIXEL, COLOR_ORDER>(leds, NEOPIXEL_COUNT);
  FastLED.setBrightness(g_brightness);
  FastLED.clear();
  FastLED.show();
  g_phase_start = millis();
}

void dockNeoPixelSetBrightness(uint8_t b) {
  g_brightness = b;
  FastLED.setBrightness(g_brightness);
}

void dockNeoPixelUpdate(NeoPixelState state, bool mouth_blocked_warn) {
  dockNeoPixelUpdateEx(state, mouth_blocked_warn, FAULT_NONE, false, false, 0.0f);
}

void dockNeoPixelUpdateEx(NeoPixelState state, bool mouth_blocked_warn,
                          DockFaultCode fault_code, bool dock_present,
                          bool mouth_blocked, float current_amps) {
  uint32_t now = millis();
  uint32_t elapsed = now - g_phase_start;

  if (state != g_last_state || fault_code != g_last_fault) {
    g_phase_start = now;
    elapsed = 0;
    g_last_state = state;
    g_last_fault = fault_code;
  }

  FastLED.clear();

  // Base brightness, dimmer in idle mode
  uint8_t baseB = g_brightness;
  if (dockIsIdleMode() && state == NP_STATE_CHARGED) {
    baseB = g_brightness / 3;
  }
  FastLED.setBrightness(baseB);

  // Map state -> colours
  switch (state) {
    case NP_STATE_NOT_DOCKED: {
      // Idle: dim amber breathing bar across all strip pixels
      uint8_t lvl = breathe(elapsed, NEOPIXEL_BREATHE_PERIOD_MS, 10, 80);
      CRGB c = CRGB(lvl, lvl / 2, 0);
      for (int i = 0; i < NEOPIXEL_STRIP_COUNT; ++i) {
        leds[i] = c;
      }
      break;
    }
    case NP_STATE_DOCKED_IDLE: {
      // Front segment = dock present, mid = amber waiting, tail = dim amber
      CRGB dockC = dock_present ? CRGB::Green : CRGB::Black;
      int midStart = NEOPIXEL_STRIP_COUNT / 3;
      int midEnd   = (NEOPIXEL_STRIP_COUNT * 2) / 3;
      for (int i = 0; i < NEOPIXEL_STRIP_COUNT; ++i) {
        if (i < midStart) {
          leds[i] = dockC;
        } else if (i < midEnd) {
          leds[i] = CRGB(255, 160, 0);
        } else {
          leds[i] = CRGB(120, 80, 0);
        }
      }
      break;
    }
    case NP_STATE_CHARGING: {
      // Orange bar scaled by current across entire strip
      float iAbs = (current_amps < 0) ? -current_amps : current_amps;
      int maxBars = NEOPIXEL_STRIP_COUNT;
      int bars = (int)(iAbs / (CURRENT_OVERCURRENT_A / (float)maxBars));
      if (bars < 1) bars = 1;
      if (bars > maxBars) bars = maxBars;
      for (int i = 0; i < NEOPIXEL_STRIP_COUNT; ++i) {
        leds[i] = CRGB(255, 140, 0);
      }
      break;
    }
    case NP_STATE_CHARGED: {
      // Solid green with slow pulse
      uint8_t lvl = breathe(elapsed, NEOPIXEL_CHARGED_PULSE_MS, 30, 120);
      FastLED.setBrightness(lvl);
      for (int i = 0; i < NEOPIXEL_STRIP_COUNT; ++i) {
        leds[i] = CRGB::Green;
      }
      break;
    }
    case NP_STATE_FAULT: {
      // Full-strip red blink for any fault
      bool on = (elapsed % 400) < 200;
      if (on) {
        for (int i = 0; i < NEOPIXEL_STRIP_COUNT; ++i) leds[i] = CRGB::Red;
      }
      break;
    }
    case NP_STATE_CALLOUT: {
      // Amber chase
      int pos = (elapsed / 80) % NEOPIXEL_STRIP_COUNT;
      for (int i = 0; i < NEOPIXEL_STRIP_COUNT; ++i) {
        int d = abs(i - pos);
        leds[i] = (d == 0) ? CRGB(255, 180, 0)
                 : (d == 1) ? CRGB(200, 100, 0)
                 : CRGB::Black;
      }
      break;
    }
  }

  // Status LED at the end
  CRGB status = CRGB::Black;
  if (state == NP_STATE_FAULT && fault_code != FAULT_NONE) {
    status = CRGB::Red;
  } else if (mouth_blocked || mouth_blocked_warn) {
    // Attention sequence: flash orange up to 10 times, then hold green.
    bool on = (elapsed % 600) < 300;
    if (!(mouth_blocked || mouth_blocked_warn)) {
      g_attention_flash_count = 0;
      g_attention_last_on = false;
    }
    if (g_attention_flash_count < 10) {
      if (on && !g_attention_last_on) {
        // Count rising edges of the blink
        g_attention_flash_count++;
      }
      g_attention_last_on = on;
      status = on ? CRGB(255, 165, 0) : CRGB::Black;
    } else {
      status = CRGB::Green;
    }
  } else if (dock_present || state == NP_STATE_NOT_DOCKED || state == NP_STATE_CHARGED) {
    g_attention_flash_count = 0;
    g_attention_last_on = false;
    status = CRGB::Green;
  }

  if (NEOPIXEL_FAULT_LED_INDEX >= 0 && NEOPIXEL_FAULT_LED_INDEX < NEOPIXEL_COUNT) {
    leds[NEOPIXEL_FAULT_LED_INDEX] = status;
  }

  FastLED.show();
}
