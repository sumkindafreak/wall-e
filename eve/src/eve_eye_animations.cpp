#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye_animations.h"
#include <math.h>
#include <stdlib.h>

static EveEyeUi* s_ui;
static float s_blinkLid = 0.f;
static uint8_t s_blinkState = 0;
static uint32_t s_blinkPhaseEnd = 0;
static uint32_t s_nextBlinkMs = 0;
static uint32_t s_nextSaccadeMs = 0;
static float s_sacDx = 0.f;
static float s_sacDy = 0.f;
static uint32_t s_dblBlinkAt = 0;

void eveEyeAnimationsInit(EveEyeUi* ui) {
  s_ui = ui;
  s_blinkLid = 0.f;
  s_blinkState = 0;
  uint32_t n = millis();
  s_nextBlinkMs = n + 2000 + (uint32_t)(rand() % 3500);
  s_nextSaccadeMs = n + 800 + (uint32_t)(rand() % 1200);
}

float eveEyeAnimationsBlinkLid(void) {
  return s_blinkLid;
}

void eveEyeAnimationsTriggerBlink(void) {
  if (s_blinkState == 0) {
    s_blinkState = 1;
    s_blinkPhaseEnd = millis() + 70;
    Serial.println(F("[EVE_FACE] Blink triggered"));
  }
}

void eveEyeAnimationsTriggerDoubleBlink(void) {
  s_dblBlinkAt = millis() + 240;
  eveEyeAnimationsTriggerBlink();
  Serial.println(F("[EVE_FACE] Double blink"));
}

void eveEyeAnimationsNudgeGaze(float dx, float dy) {
  s_sacDx += dx;
  s_sacDy += dy;
}

void eveEyeAnimationsSetSleepClosed(bool closed) {
  s_blinkLid = closed ? 1.f : 0.f;
  s_blinkState = 0;
}

void eveEyeAnimationsWakeOpen(void) {
  s_blinkState = 3;
  s_blinkPhaseEnd = millis() + 900;
}

static void mergedMicroGazeDecay(float dtSec) {
  float k = 1.f - 0.35f * fminf(dtSec * 30.f, 1.f);
  s_sacDx *= k;
  s_sacDy *= k;
}

static void updateBlink(uint32_t now) {
  switch (s_blinkState) {
    case 0:
      break;
    case 1:
      s_blinkLid = 1.f;
      if (now >= s_blinkPhaseEnd) {
        s_blinkState = 2;
        s_blinkPhaseEnd = now + 140;
      }
      break;
    case 2:
      s_blinkLid = 0.f;
      if (now >= s_blinkPhaseEnd) {
        s_blinkState = 0;
        s_nextBlinkMs = now + 1800 + (uint32_t)(rand() % 4000);
      }
      break;
    case 3: {
      float remain = (float)((int32_t)(s_blinkPhaseEnd - now));
      float prog = 1.f - remain / 900.f;
      if (prog < 0.f) {
        prog = 0.f;
      }
      if (prog > 1.f) {
        prog = 1.f;
      }
      s_blinkLid = 1.f - prog;
      if (now >= s_blinkPhaseEnd) {
        s_blinkLid = 0.f;
        s_blinkState = 0;
      }
      break;
    }
    default:
      s_blinkState = 0;
      break;
  }

  if (s_blinkState == 0 && s_dblBlinkAt != 0 && (int32_t)(now - s_dblBlinkAt) >= 0) {
    s_dblBlinkAt = 0;
    eveEyeAnimationsTriggerBlink();
  }

  if (s_blinkState == 0 && now >= s_nextBlinkMs) {
    eveEyeAnimationsTriggerBlink();
  }
}

void eveEyeAnimationsTick(uint32_t nowMs, float dtSec) {
  (void)dtSec;
  updateBlink(nowMs);

  if (nowMs >= s_nextSaccadeMs) {
    s_nextSaccadeMs = nowMs + 2200 + (uint32_t)(rand() % 2800);
    s_sacDx = ((float)(rand() % 255) / 255.f - 0.5f) * 0.08f;
    s_sacDy = ((float)(rand() % 255) / 255.f - 0.5f) * 0.06f;
  }

  if (s_ui && s_ui->scan_bar) {
    int32_t vh = lv_obj_get_height(s_ui->visor);
    if (vh > 60) {
      float wave = sinf((float)nowMs * 0.0011f) * 0.5f + 0.5f;
      int32_t y = 20 + (int32_t)(wave * (vh - 48));
      lv_obj_set_y(s_ui->scan_bar, y);
    }
  }

  mergedMicroGazeDecay(dtSec);
}

void eveEyeAnimationsGetMicroGaze(float* dx, float* dy) {
  if (dx) {
    *dx = s_sacDx;
  }
  if (dy) {
    *dy = s_sacDy;
  }
}

#endif /* EVE_ENABLE_EYES */
