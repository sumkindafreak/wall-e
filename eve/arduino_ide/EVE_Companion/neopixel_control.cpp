#include "neopixel_control.h"

#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "eve_output_ids.h"
#include "eve_safety.h"

#define EVE_ONBOARD_ON (EVE_ONBOARD_RGB_HEARTBEAT && (EVE_ONBOARD_RGB_PIN >= 0))
#define EVE_STRIP_ON (EVE_ENABLE_NEOPIXEL && (EVE_NEOPIXEL_PIN >= 0) && (EVE_NEOPIXEL_COUNT > 0))

#if EVE_ONBOARD_ON
static Adafruit_NeoPixel s_onboard(1, EVE_ONBOARD_RGB_PIN, NEO_GRB + NEO_KHZ800);
#endif
#if EVE_STRIP_ON
static Adafruit_NeoPixel s_strip(EVE_NEOPIXEL_COUNT, EVE_NEOPIXEL_PIN, EVE_NEOPIXEL_TYPE);
#endif

static uint8_t s_base = EVE_GLOW_WAITING;
static int16_t s_operator = -1;
static int16_t s_overlay = -1;
static uint32_t s_overlayUntil = 0; /* 0 = until released */
static uint8_t s_shown = EVE_GLOW_WAITING;
static uint32_t s_lastFrameMs = 0;

struct Rgb {
  uint8_t r, g, b;
};

/** Triangle wave 0..255 over periodMs. */
static uint8_t tri(uint32_t now, uint32_t periodMs) {
  const uint32_t ph = now % periodMs;
  const uint32_t half = periodMs / 2u;
  return (uint8_t)(ph < half ? ph * 255u / half : (periodMs - ph) * 255u / half);
}

/** Breathing between floor and 255 (keeps EVE's glow from going fully dark). */
static uint8_t breathe(uint32_t now, uint32_t periodMs, uint8_t floor) {
  return (uint8_t)(floor + ((uint32_t)(255u - floor) * tri(now, periodMs)) / 255u);
}

static Rgb scale(Rgb c, uint8_t k) {
  return {(uint8_t)((uint16_t)c.r * k / 255u), (uint8_t)((uint16_t)c.g * k / 255u), (uint8_t)((uint16_t)c.b * k / 255u)};
}

/** Colour of one pixel. idx/count let the strip animate (scan sweep, sparkle). */
static Rgb colourFor(uint8_t p, uint32_t now, uint16_t idx, uint16_t count) {
  const Rgb blue = {0, 70, 255};
  switch (p) {
    case EVE_GLOW_IDLE: return scale(blue, breathe(now, 3200, 60));
    case EVE_GLOW_LINKED: return scale(blue, breathe(now, 2400, 110));
    case EVE_GLOW_WAITING: return scale(blue, (uint8_t)(breathe(now, 4200, 20) / 3u));
    case EVE_GLOW_CRITICAL: return scale({255, 0, 0}, breathe(now, 1200, 30));
    case EVE_GLOW_STANDBY_BEACON: return scale({0, 255, 40}, (uint8_t)(breathe(now, 5200, 10) / 2u));
    case EVE_GLOW_INTERACT: return scale({120, 0, 255}, breathe(now, 2600, 90));
    case EVE_GLOW_ESCORT: return {70, 150, 255};
    case EVE_GLOW_LOW_POWER: return scale({255, 80, 0}, (uint8_t)(breathe(now, 3200, 20) / 2u));
    case EVE_GLOW_CHARGING: return scale({0, 255, 140}, breathe(now, 2600, 50));
    case EVE_GLOW_FULL: return {190, 210, 255};
    case EVE_GLOW_FAULT: {
      const uint32_t ph = now % 1000u; /* on-off-on, long pause */
      return (ph < 120u || (ph >= 240u && ph < 360u)) ? Rgb{255, 0, 0} : Rgb{0, 0, 0};
    }
    case EVE_GLOW_STOPPED: return {90, 0, 0};
    case EVE_GLOW_SCAN: {
      if (count <= 1) return scale(blue, tri(now, 900));
      /* A soft spot sweeping end to end — an animation, not a sensor. */
      const uint32_t span = (uint32_t)(count - 1) * 2u;
      const uint32_t pos = (now / 70u) % span;
      const uint32_t head = pos < (uint32_t)count ? pos : span - pos;
      const uint32_t d = head > idx ? head - idx : idx - head;
      return d == 0 ? blue : (d == 1 ? scale(blue, 70) : Rgb{0, 0, 8});
    }
    case EVE_GLOW_PLAY: {
      const uint32_t seed = (now / 120u) * 2654435761u + idx * 40503u;
      return ((seed >> 13) % 5u == 0u) ? Rgb{170, 200, 255} : scale(blue, 80);
    }
    case EVE_GLOW_PROTECT: return {0, 90, 255};
    case EVE_GLOW_OFF:
    default: return {0, 0, 0};
  }
}

static uint8_t pick(uint32_t now) {
  if (eveSafetyStopLatched()) return EVE_GLOW_STOPPED;
  if (eveSafetyHasBlockingFault()) return EVE_GLOW_FAULT;
  if (s_overlay >= 0) {
    if (s_overlayUntil != 0 && (int32_t)(now - s_overlayUntil) >= 0) {
      s_overlay = -1;
    } else {
      return (uint8_t)s_overlay;
    }
  }
  if (s_operator >= 0) return (uint8_t)s_operator;
  return s_base;
}

void neopixelInit(void) {
#if EVE_ONBOARD_ON
  s_onboard.begin();
  s_onboard.setBrightness(EVE_ONBOARD_RGB_BRIGHTNESS);
  s_onboard.clear();
  s_onboard.show();
  Serial.printf("[EVE][RGB] onboard LED on GPIO%d\n", EVE_ONBOARD_RGB_PIN);
#endif
#if EVE_STRIP_ON
  s_strip.begin();
  s_strip.setBrightness(EVE_NEOPIXEL_BRIGHTNESS);
  s_strip.clear();
  s_strip.show();
  Serial.printf("[EVE][NeoPixel] strip: %d px on GPIO%d, brightness %u\n", EVE_NEOPIXEL_COUNT, EVE_NEOPIXEL_PIN,
                (unsigned)EVE_NEOPIXEL_BRIGHTNESS);
#elif EVE_ENABLE_NEOPIXEL
  Serial.println(F("[EVE][NeoPixel] enabled but EVE_NEOPIXEL_PIN is -1 - strip stays off"));
#else
  Serial.println(F("[EVE][NeoPixel] strip disabled (config)"));
#endif
}

void neopixelTick(void) {
  const uint32_t now = millis();
  if ((uint32_t)(now - s_lastFrameMs) < EVE_NEOPIXEL_FRAME_MS) return;
  s_lastFrameMs = now;
  const uint8_t p = pick(now);
  if (p != s_shown) {
#if EVE_SERIAL_VERBOSE_LOGS
    Serial.printf("[EVE][GLOW] %s -> %s\n", eveGlowName(s_shown), eveGlowName(p));
#endif
    s_shown = p;
  }
#if EVE_ONBOARD_ON
  const Rgb c = colourFor(p, now, 0, 1);
  s_onboard.setPixelColor(0, c.r, c.g, c.b);
  s_onboard.show();
#endif
#if EVE_STRIP_ON
  for (uint16_t i = 0; i < (uint16_t)EVE_NEOPIXEL_COUNT; i++) {
    const Rgb c = colourFor(p, now, i, (uint16_t)EVE_NEOPIXEL_COUNT);
    s_strip.setPixelColor(i, c.r, c.g, c.b);
  }
  s_strip.show();
#endif
}

void neopixelSetPattern(uint8_t pattern) { neopixelSetOperator(pattern < EVE_GLOW_COUNT ? (int8_t)pattern : (int8_t)-1); }

void neopixelSetOperator(int8_t pattern) { s_operator = (pattern >= 0 && pattern < EVE_GLOW_COUNT) ? pattern : -1; }

void neopixelSetBase(uint8_t pattern) {
  if (pattern < EVE_GLOW_COUNT) s_base = pattern;
}

void neopixelSetOverlay(uint8_t pattern, uint32_t ms) {
  if (pattern == 0xFF || pattern >= EVE_GLOW_COUNT) {
    s_overlay = -1;
    return;
  }
  s_overlay = pattern;
  s_overlayUntil = ms ? (millis() + ms) | 1u : 0;
}

uint8_t neopixelCurrentPattern(void) { return s_shown; }

bool neopixelStripActive(void) {
#if EVE_STRIP_ON
  return true;
#else
  return false;
#endif
}

bool neopixelOnboardActive(void) {
#if EVE_ONBOARD_ON
  return true;
#else
  return false;
#endif
}
