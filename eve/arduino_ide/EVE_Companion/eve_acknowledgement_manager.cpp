#include "eve_acknowledgement_manager.h"
#include "config.h"
#include "eve_spatial_awareness.h"
#include <math.h>
#include <stdlib.h>

#if EVE_ENABLE_EYES
#include "eve_expression_state.h"
#endif

static uint32_t s_coolUntil = 0;
static uint32_t s_lastRollMs = 0;

void eveAcknowledgementInit(void) {
  s_coolUntil = 0;
  s_lastRollMs = 0;
}

static bool confusedVisual(void) {
#if EVE_ENABLE_EYES
  EveExpressionId e = eveExpressionGetCurrent();
  return (e == EVE_EXPR_CONFUSED);
#else
  return false;
#endif
}

bool eveAcknowledgementEvaluate(const EveTargetSnapshot* snap, uint32_t nowMs, uint8_t spatialFlags) {
  if (!snap) {
    return false;
  }

  if (spatialFlags & EVE_SPATIAL_FLAG_SLEEP) {
    return false;
  }
  if (spatialFlags & EVE_SPATIAL_FLAG_CONFUSED) {
    return false;
  }
  if (confusedVisual()) {
    return false;
  }

  if (snap->zone == EVE_TARGET_MODEL_NONE || snap->zone == EVE_TARGET_MODEL_UNCERTAIN) {
    return false;
  }

  if (snap->confidencePct < 22) {
    return false;
  }

  if (nowMs < s_coolUntil) {
    Serial.println(F("[EVE_TOF] Cooldown active, skipping visible response"));
    return false;
  }

  float p = 0.18f;
  if (spatialFlags & EVE_SPATIAL_FLAG_LOW_BATTERY) {
    p *= 0.45f;
  }
  if (spatialFlags & EVE_SPATIAL_FLAG_ALERT) {
    p *= 1.35f;
  }
  if (spatialFlags & EVE_SPATIAL_FLAG_ATTACHED) {
    p *= 0.85f;
  }
  if (snap->stableStrong) {
    p = fminf(0.42f, p + 0.12f);
  }

  if (p > 0.55f) {
    p = 0.55f;
  }

  int r = rand() % 10000;
  if ((float)r >= p * 10000.f) {
    return false;
  }

  uint32_t gap = 2200u + (uint32_t)(rand() % 2800);
  s_coolUntil = nowMs + gap;
  s_lastRollMs = nowMs;
  (void)s_lastRollMs;

  Serial.println(F("[EVE_TOF] Ack roll passed (probabilistic)"));
  return true;
}
