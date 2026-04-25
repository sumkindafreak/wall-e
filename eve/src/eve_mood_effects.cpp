#include "eve_mood_effects.h"

#include "config.h"

#include <math.h>
#include <stdlib.h>

struct Rgb8 {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

static Rgb8 s_cur = {24, 24, 40};
static Rgb8 s_tgt = {24, 24, 40};
static uint32_t s_pulsePhaseMs;
static uint32_t s_lastLogMs;

static void patternToRgb(uint8_t pattern, Rgb8* out) {
  if (!out) {
    return;
  }
  switch (pattern % 6u) {
    case 1u:
      out->r = 0;
      out->g = 96;
      out->b = 180;
      break;
    case 2u:
      out->r = 180;
      out->g = 32;
      out->b = 0;
      break;
    case 3u:
      out->r = 140;
      out->g = 0;
      out->b = 160;
      break;
    case 4u:
      out->r = 0;
      out->g = 200;
      out->b = 60;
      break;
    case 5u:
      out->r = 200;
      out->g = 200;
      out->b = 40;
      break;
    default:
      out->r = 24;
      out->g = 24;
      out->b = 40;
      break;
  }
}

void eveMoodEffectsInit(void) {
  s_cur = s_tgt = {24, 24, 40};
  s_pulsePhaseMs = millis();
  s_lastLogMs = 0;
}

void eveMoodEffectsOnPattern(uint8_t pattern) {
  patternToRgb(pattern, &s_tgt);
}

static uint8_t moodStepChannel(uint8_t c, uint8_t t) {
  if (c == t) {
    return c;
  }
  const int d = (int)t - (int)c;
  const int step = (d > 0) ? 1 : -1;
  return (uint8_t)((int)c + step);
}

void eveMoodEffectsTick(uint32_t nowMs) {
  s_cur.r = moodStepChannel(s_cur.r, s_tgt.r);
  s_cur.g = moodStepChannel(s_cur.g, s_tgt.g);
  s_cur.b = moodStepChannel(s_cur.b, s_tgt.b);

  const uint32_t ph = nowMs - s_pulsePhaseMs;
  if (ph > 6000u + (uint32_t)(rand() % 5000)) {
    s_pulsePhaseMs = nowMs;
  }

  const float breathe = 0.88f + 0.12f * sinf((float)nowMs * 0.0021f);
  (void)breathe;

#if EVE_ENABLE_NEOPIXEL
  if ((uint32_t)(nowMs - s_lastLogMs) > 2500u) {
    s_lastLogMs = nowMs;
    const uint8_t pr = (uint8_t)constrain((int)lroundf((float)s_cur.r * breathe), 0, 255);
    const uint8_t pg = (uint8_t)constrain((int)lroundf((float)s_cur.g * breathe), 0, 255);
    const uint8_t pb = (uint8_t)constrain((int)lroundf((float)s_cur.b * breathe), 0, 255);
    Serial.printf("[EVE][mood] rgb %u %u %u (fade/pulse; wire pixels to apply)\n", (unsigned)pr, (unsigned)pg,
                  (unsigned)pb);
  }
#else
  (void)breathe;
#endif
}
