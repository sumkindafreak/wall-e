#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye_animations.h"
#include <math.h>
#include <stdlib.h>

static EveEyeUi* s_ui;
static float s_lidL = 0.f;
static float s_lidR = 0.f;
static uint8_t s_blinkState = 0;
static uint32_t s_blinkPhaseEnd = 0;
static uint32_t s_nextBlinkMs = 0;
static uint32_t s_nextSaccadeMs = 0;
static float s_sacDx = 0.f;
static float s_sacDy = 0.f;
static uint32_t s_dblBlinkAt = 0;
static bool s_slowBlink = false;
static bool s_winkLeft = false;
static bool s_winkRight = false;
static float s_squintOverlay = 0.f;
static float s_widenOverlay = 0.f;

void eveEyeAnimationsInit(EveEyeUi* ui) {
  s_ui = ui;
  s_lidL = s_lidR = 0.f;
  s_blinkState = 0;
  uint32_t n = millis();
  s_nextBlinkMs = n + 2000 + (uint32_t)(rand() % 3500);
  s_nextSaccadeMs = n + 800 + (uint32_t)(rand() % 1200);
  s_slowBlink = false;
  s_winkLeft = s_winkRight = false;
  s_squintOverlay = s_widenOverlay = 0.f;
}

void eveEyeAnimationsGetLids(float* left, float* right) {
  if (left) {
    *left = s_lidL;
  }
  if (right) {
    *right = s_lidR;
  }
}

float eveEyeAnimationsSquintOverlay(void) {
  return s_squintOverlay;
}

float eveEyeAnimationsWidenOverlay(void) {
  return s_widenOverlay;
}

void eveEyeAnimationsTriggerBlink(void) {
  if (s_blinkState == 0) {
    s_blinkState = 1;
    s_blinkPhaseEnd = millis() + 70;
    s_slowBlink = false;
    s_winkLeft = s_winkRight = false;
    Serial.println(F("[EVE_FACE] Blink triggered"));
  }
}

void eveEyeAnimationsTriggerDoubleBlink(void) {
  s_dblBlinkAt = millis() + 240;
  eveEyeAnimationsTriggerBlink();
  Serial.println(F("[EVE_FACE] Double blink"));
}

void eveEyeAnimationsTriggerSlowBlink(void) {
  if (s_blinkState == 0) {
    s_slowBlink = true;
    s_blinkState = 1;
    s_blinkPhaseEnd = millis() + 180;
    s_winkLeft = s_winkRight = false;
    Serial.println(F("[EVE_FACE] Slow blink"));
  }
}

void eveEyeAnimationsTriggerWink(bool leftEye) {
  if (s_blinkState != 0) {
    return;
  }
  s_winkLeft = leftEye;
  s_winkRight = !leftEye;
  s_blinkState = 1;
  s_blinkPhaseEnd = millis() + 90;
  s_slowBlink = false;
  Serial.println(F("[EVE_FACE] Wink"));
}

void eveEyeAnimationsTriggerSquint(float amount) {
  if (amount < 0.f) {
    amount = 0.f;
  }
  if (amount > 1.f) {
    amount = 1.f;
  }
  s_squintOverlay = fmaxf(s_squintOverlay, amount);
}

void eveEyeAnimationsTriggerWiden(float amount) {
  if (amount < 0.f) {
    amount = 0.f;
  }
  if (amount > 1.f) {
    amount = 1.f;
  }
  s_widenOverlay = fmaxf(s_widenOverlay, amount);
}

void eveEyeAnimationsNudgeGaze(float dx, float dy) {
  s_sacDx += dx;
  s_sacDy += dy;
}

void eveEyeAnimationsSetSleepClosed(bool closed) {
  s_lidL = s_lidR = closed ? 1.f : 0.f;
  s_blinkState = 0;
  s_winkLeft = s_winkRight = false;
}

void eveEyeAnimationsWakeOpen(void) {
  s_blinkState = 3;
  s_blinkPhaseEnd = millis() + 900;
  s_winkLeft = s_winkRight = false;
}

static void mergedMicroGazeDecay(float dtSec) {
  float k = 1.f - 0.35f * fminf(dtSec * 30.f, 1.f);
  s_sacDx *= k;
  s_sacDy *= k;
  s_squintOverlay *= (1.f - 0.12f * fminf(dtSec * 60.f, 1.f));
  s_widenOverlay *= (1.f - 0.1f * fminf(dtSec * 60.f, 1.f));
}

static void setBothLids(float v) {
  s_lidL = s_lidR = v;
}

static void updateBlink(uint32_t now) {
  uint32_t closeMs = s_slowBlink ? 180u : 70u;
  uint32_t openMs = s_slowBlink ? 320u : 140u;

  switch (s_blinkState) {
    case 0:
      break;
    case 1:
      if (s_winkLeft || s_winkRight) {
        s_lidL = s_winkLeft ? 1.f : 0.f;
        s_lidR = s_winkRight ? 1.f : 0.f;
      } else {
        setBothLids(1.f);
      }
      if (now >= s_blinkPhaseEnd) {
        s_blinkState = 2;
        s_blinkPhaseEnd = now + openMs;
      }
      break;
    case 2:
      setBothLids(0.f);
      s_winkLeft = s_winkRight = false;
      if (now >= s_blinkPhaseEnd) {
        s_blinkState = 0;
        s_slowBlink = false;
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
      setBothLids(1.f - prog);
      if (now >= s_blinkPhaseEnd) {
        setBothLids(0.f);
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
}

void eveEyeAnimationsTick(uint32_t nowMs, float dtSec) {
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
