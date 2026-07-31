#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_gaze_engine.h"
#include "eve_expression_state.h"
#include <math.h>
#include <stdlib.h>

static float s_curNx = 0.5f;
static float s_curNy = 0.48f;
static float s_tgtNx = 0.5f;
static float s_tgtNy = 0.48f;
static uint32_t s_holdUntil = 0;
static uint32_t s_returnUntil = 0;
static bool s_scriptActive = false;
static bool s_yield = false;

static float clamp01(float v) {
  if (v < 0.f) {
    return 0.f;
  }
  if (v > 1.f) {
    return 1.f;
  }
  return v;
}

static void applyTarget(void) {
  eveExpressionSetTargetGaze(s_curNx, s_curNy);
}

void eveGazeInit(void) {
  s_curNx = s_tgtNx = 0.5f;
  s_curNy = s_tgtNy = 0.48f;
  s_holdUntil = 0;
  s_returnUntil = 0;
  s_scriptActive = false;
  s_yield = false;
}

void eveGazeYield(void) {
  s_yield = true;
  s_scriptActive = false;
}

bool eveGazeScriptOwnsTarget(void) {
  return s_scriptActive && !s_yield;
}

bool eveGazeIsBusy(void) {
  if (!s_scriptActive) {
    return false;
  }
  float dx = s_tgtNx - s_curNx;
  float dy = s_tgtNy - s_curNy;
  return (fabsf(dx) > 0.015f || fabsf(dy) > 0.015f || millis() < s_holdUntil);
}

static void setDirectionTarget(EveGazeDirection dir) {
  switch (dir) {
    case EVE_GAZE_LEFT:
      s_tgtNx = 0.18f;
      s_tgtNy = 0.48f;
      break;
    case EVE_GAZE_RIGHT:
      s_tgtNx = 0.82f;
      s_tgtNy = 0.48f;
      break;
    case EVE_GAZE_UP:
      s_tgtNx = 0.5f;
      s_tgtNy = 0.22f;
      break;
    case EVE_GAZE_DOWN:
      s_tgtNx = 0.5f;
      s_tgtNy = 0.72f;
      break;
    default:
      s_tgtNx = 0.5f;
      s_tgtNy = 0.48f;
      break;
  }
}

void eveGazeLook(EveGazeDirection dir, uint32_t holdMs) {
  s_yield = false;
  s_scriptActive = true;
  setDirectionTarget(dir);
  uint32_t now = millis();
  s_holdUntil = now + (holdMs ? holdMs : 900u + (uint32_t)(rand() % 600));
  s_returnUntil = 0;
}

void eveGazeTrackNormalized(float nx, float ny) {
  s_yield = false;
  s_scriptActive = true;
  s_tgtNx = clamp01(nx);
  s_tgtNy = clamp01(ny);
  s_holdUntil = millis() + 400u;
  s_returnUntil = 0;
}

void eveGazeTrackZone(EveTargetModel zone) {
  switch (zone) {
    case EVE_TARGET_MODEL_LEFT:
      eveGazeTrackNormalized(0.22f, 0.46f);
      break;
    case EVE_TARGET_MODEL_RIGHT:
      eveGazeTrackNormalized(0.78f, 0.46f);
      break;
    case EVE_TARGET_MODEL_CENTER:
    case EVE_TARGET_MODEL_MULTI:
      eveGazeTrackNormalized(0.5f, 0.44f);
      break;
    default:
      break;
  }
}

void eveGazeReturnCenter(uint32_t durationMs) {
  s_yield = false;
  s_scriptActive = true;
  s_tgtNx = 0.5f;
  s_tgtNy = 0.48f;
  s_returnUntil = millis() + (durationMs ? durationMs : 700u);
  s_holdUntil = 0;
}

void eveGazeTick(uint32_t nowMs, float dtSec) {
  if (s_yield) {
    return;
  }

  if (s_returnUntil != 0 && nowMs >= s_returnUntil) {
    s_returnUntil = 0;
    s_scriptActive = false;
    s_tgtNx = 0.5f;
    s_tgtNy = 0.48f;
  }

  if (s_scriptActive && s_holdUntil != 0 && nowMs >= s_holdUntil && s_returnUntil == 0) {
    s_tgtNx = 0.5f;
    s_tgtNy = 0.48f;
    s_holdUntil = 0;
    s_returnUntil = nowMs + 650u;
  }

  float ease = 0.08f + 0.22f * fminf(dtSec * 60.f, 1.f);
  s_curNx += (s_tgtNx - s_curNx) * ease;
  s_curNy += (s_tgtNy - s_curNy) * ease;

  if (s_scriptActive) {
    applyTarget();
  }
}

#else /* !EVE_ENABLE_EYES */

#include "eve_gaze_engine.h"

void eveGazeInit(void) {}
void eveGazeTick(uint32_t, float) {}
void eveGazeLook(EveGazeDirection, uint32_t) {}
void eveGazeTrackNormalized(float, float) {}
void eveGazeReturnCenter(uint32_t) {}
void eveGazeTrackZone(EveTargetModel) {}
bool eveGazeIsBusy(void) {
  return false;
}
bool eveGazeScriptOwnsTarget(void) {
  return false;
}
void eveGazeYield(void) {}

#endif /* EVE_ENABLE_EYES */
