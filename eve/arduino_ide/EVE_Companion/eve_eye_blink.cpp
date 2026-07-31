#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye_blink.h"
#include <math.h>
#include <stdlib.h>

typedef struct {
  float lid;
  uint8_t state;
  uint32_t phaseEnd;
  uint32_t nextBlinkMs;
  uint32_t dblAt;
  bool slow;
} BlinkChannel;

static BlinkChannel s_ch[2];
static float s_squint = 0.f;
static float s_widen = 0.f;
static float s_sacDx = 0.f;
static float s_sacDy = 0.f;
static uint32_t s_nextSaccadeMs = 0;

static BlinkChannel* ch(EveEyeSide side) {
  return &s_ch[side == EVE_EYE_SIDE_RIGHT ? 1 : 0];
}

void eveEyeBlinkInit(void) {
  uint32_t n = millis();
  for (int i = 0; i < 2; ++i) {
    s_ch[i].lid = 0.f;
    s_ch[i].state = 0;
    s_ch[i].slow = false;
    s_ch[i].dblAt = 0;
    uint32_t skew = (i == 0) ? 0u : (90u + (uint32_t)(rand() % 140));
    s_ch[i].nextBlinkMs = n + 2000u + skew + (uint32_t)(rand() % 3500);
  }
  s_nextSaccadeMs = n + 800 + (uint32_t)(rand() % 1200);
  s_squint = s_widen = 0.f;
  s_sacDx = s_sacDy = 0.f;
}

float eveEyeBlinkLid(EveEyeSide side) {
  return ch(side)->lid;
}

static void startBlink(EveEyeSide side, bool slow) {
  BlinkChannel* c = ch(side);
  if (c->state != 0) {
    return;
  }
  c->slow = slow;
  c->state = 1;
  c->phaseEnd = millis() + (slow ? 180u : 70u);
}

void eveEyeBlinkTrigger(EveEyeSide side) {
  startBlink(side, false);
}

void eveEyeBlinkTriggerSlow(EveEyeSide side) {
  startBlink(side, true);
}

void eveEyeBlinkTriggerDouble(EveEyeSide side) {
  ch(side)->dblAt = millis() + 220u + (uint32_t)(rand() % 80);
  eveEyeBlinkTrigger(side);
}

void eveEyeBlinkTriggerWink(EveEyeSide side) {
  eveEyeBlinkTrigger(side);
}

void eveEyeBlinkTriggerRandom(void) {
  EveEyeSide side = (rand() & 1) ? EVE_EYE_SIDE_RIGHT : EVE_EYE_SIDE_LEFT;
  eveEyeBlinkTrigger(side);
}

void eveEyeBlinkTriggerSquint(float amount) {
  if (amount < 0.f) {
    amount = 0.f;
  }
  if (amount > 1.f) {
    amount = 1.f;
  }
  s_squint = fmaxf(s_squint, amount);
}

void eveEyeBlinkTriggerWiden(float amount) {
  if (amount < 0.f) {
    amount = 0.f;
  }
  if (amount > 1.f) {
    amount = 1.f;
  }
  s_widen = fmaxf(s_widen, amount);
}

float eveEyeBlinkSquintOverlay(void) {
  return s_squint;
}

float eveEyeBlinkWidenOverlay(void) {
  return s_widen;
}

void eveEyeBlinkNudgeGaze(float dx, float dy) {
  s_sacDx += dx;
  s_sacDy += dy;
}

void eveEyeBlinkGetMicroGaze(float* dx, float* dy) {
  if (dx) {
    *dx = s_sacDx;
  }
  if (dy) {
    *dy = s_sacDy;
  }
}

void eveEyeBlinkSetSleepClosed(bool closed) {
  s_ch[0].lid = s_ch[1].lid = closed ? 1.f : 0.f;
  s_ch[0].state = s_ch[1].state = 0;
}

void eveEyeBlinkWakeOpen(void) {
  s_ch[0].state = s_ch[1].state = 3;
  s_ch[0].phaseEnd = s_ch[1].phaseEnd = millis() + 900u;
}

static void updateChannel(BlinkChannel* c, uint32_t now) {
  uint32_t openMs = c->slow ? 320u : 140u;
  switch (c->state) {
    case 0:
      break;
    case 1:
      c->lid = 1.f;
      if (now >= c->phaseEnd) {
        c->state = 2;
        c->phaseEnd = now + openMs;
      }
      break;
    case 2:
      c->lid = 0.f;
      if (now >= c->phaseEnd) {
        c->state = 0;
        c->slow = false;
        c->nextBlinkMs = now + 1800u + (uint32_t)(rand() % 4000);
      }
      break;
    case 3: {
      float remain = (float)((int32_t)(c->phaseEnd - now));
      float prog = 1.f - remain / 900.f;
      if (prog < 0.f) {
        prog = 0.f;
      }
      if (prog > 1.f) {
        prog = 1.f;
      }
      c->lid = 1.f - prog;
      if (now >= c->phaseEnd) {
        c->lid = 0.f;
        c->state = 0;
      }
      break;
    }
    default:
      c->state = 0;
      break;
  }

  if (c->state == 0 && c->dblAt != 0 && (int32_t)(now - c->dblAt) >= 0) {
    c->dblAt = 0;
    c->state = 1;
    c->phaseEnd = now + 70;
  }
}

void eveEyeBlinkTick(uint32_t nowMs, float dtSec) {
  updateChannel(&s_ch[0], nowMs);
  updateChannel(&s_ch[1], nowMs);

  if (s_ch[0].state == 0 && nowMs >= s_ch[0].nextBlinkMs && (rand() % 5) != 0) {
    eveEyeBlinkTrigger(EVE_EYE_SIDE_LEFT);
  }
  if (s_ch[1].state == 0 && nowMs >= s_ch[1].nextBlinkMs) {
    eveEyeBlinkTrigger(EVE_EYE_SIDE_RIGHT);
  }

  if (nowMs >= s_nextSaccadeMs) {
    s_nextSaccadeMs = nowMs + 2200 + (uint32_t)(rand() % 2800);
    s_sacDx = ((float)(rand() % 255) / 255.f - 0.5f) * 0.08f;
    s_sacDy = ((float)(rand() % 255) / 255.f - 0.5f) * 0.06f;
  }

  float k = 1.f - 0.35f * fminf(dtSec * 30.f, 1.f);
  s_sacDx *= k;
  s_sacDy *= k;
  s_squint *= (1.f - 0.12f * fminf(dtSec * 60.f, 1.f));
  s_widen *= (1.f - 0.1f * fminf(dtSec * 60.f, 1.f));
}

#endif /* EVE_ENABLE_EYES */
