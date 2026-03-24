/*
 * FastLED 30x4 matrix implementation. Day/night from NTP; no LDR.
 */
#if defined(LED_ENABLED) && (LED_ENABLED) == 1

#include "led_fastled.h"
#include "config.h"
#include <FastLED.h>
#include <Arduino.h>

#define STRIP_TYPE WS2812B
#define STRIP_ORDER GRB

static CRGB leds[LED_COUNT];
static uint8_t g_brightness = LED_BRIGHTNESS_DEFAULT;
static uint8_t g_mode = LED_AUTO;
static uint8_t g_manual_r = 255, g_manual_g = 220, g_manual_b = 180;

int ledXY(int x, int y) {
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return -1;
#if MATRIX_ZIGZAG
  if (y & 1) x = MATRIX_WIDTH - 1 - x;
#endif
  return y * MATRIX_WIDTH + x;
}

void ledBegin(void) {
  FastLED.addLeds<STRIP_TYPE, PIN_LED, STRIP_ORDER>(leds, LED_COUNT);
  FastLED.setBrightness(g_brightness);
  FastLED.clear();
  FastLED.show();
}

void ledSetBrightness(uint8_t b) {
  g_brightness = b;
  FastLED.setBrightness(g_brightness);
}

void ledSetMode(uint8_t mode) {
  if (mode <= LED_AUTO) g_mode = mode;
}

void ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
  g_manual_r = r;
  g_manual_g = g;
  g_manual_b = b;
}

uint8_t ledGetBrightness(void) { return g_brightness; }
uint8_t ledGetMode(void) { return g_mode; }

static void effectDay(void) {
  /* Warm white / row gradient: brighter top, slightly dimmer lower rows */
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    uint8_t dim = 255 - (y * 25);  /* 255, 230, 205, 180 */
    if (dim < 100) dim = 100;
    CRGB c(dim, (dim * 9) / 10, (dim * 6) / 10);
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      int i = ledXY(x, y);
      if (i >= 0) leds[i] = c;
    }
  }
  FastLED.setBrightness(g_brightness);
  FastLED.show();
}

static void effectNight(void) {
  /* Dim blue / moonlight */
  CRGB c(30, 40, 80);
  for (int i = 0; i < LED_COUNT; i++) leds[i] = c;
  FastLED.setBrightness((g_brightness * 30) / 100);  /* 30% at night */
  FastLED.show();
}

static void effectManual(void) {
  CRGB c(g_manual_r, g_manual_g, g_manual_b);
  for (int i = 0; i < LED_COUNT; i++) leds[i] = c;
  FastLED.setBrightness(g_brightness);
  FastLED.show();
}

static void effectOff(void) {
  FastLED.clear();
  FastLED.show();
}

void ledUpdate(uint8_t hour, uint8_t minute, bool timeSynced, uint16_t dayStartMin, uint16_t dayEndMin) {
  switch (g_mode) {
    case LED_OFF:
      effectOff();
      return;
    case LED_MANUAL:
      effectManual();
      return;
    case LED_AUTO:
      break;
    default:
      effectOff();
      return;
  }

  /* AUTO: use time to choose day vs night. When not synced, treat as night. */
  if (!timeSynced) {
    effectNight();
    return;
  }
  uint16_t nowMin = (uint16_t)hour * 60 + minute;
  if (nowMin >= dayStartMin && nowMin < dayEndMin)
    effectDay();
  else
    effectNight();
}

#endif /* LED_ENABLED */
