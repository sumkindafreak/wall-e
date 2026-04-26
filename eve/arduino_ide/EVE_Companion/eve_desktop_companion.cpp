#include "eve_desktop_companion.h"

#include "audio_control.h"
#include "config.h"
#include "eyes_control.h"
#include "neopixel_control.h"
#include "servo_control.h"

#if EVE_ENABLE_EYES
#include "eve_expression_state.h"
#endif

#if EVE_ENABLE_TOF
#include "eve_target_tracker.h"
#include "eve_tof_manager.h"
#endif

#include <stdlib.h>

static bool s_active = false;
static bool s_charging = false;
static uint32_t s_lastIdleMs = 0;
static uint32_t s_lastInteractionMs = 0;
static uint32_t s_lastAudioMs = 0;
static uint32_t s_nextIdleMs = 0;
static bool s_lookRight = false;

static void requestExpression_(uint8_t legacyMode) {
  eyesSetMode(legacyMode);
}

static void setGaze_(float gx, float gy) {
#if EVE_ENABLE_EYES
  eveExpressionSetTargetGaze(gx, gy);
#else
  (void)gx;
  (void)gy;
#endif
}

static void requestDockExpression_(uint32_t nowMs, bool interacting) {
#if EVE_ENABLE_EYES
  if (interacting) {
    eveExpressionRequest(EVE_EXPR_HAPPY, 1800u);
  } else if ((nowMs / 7000u) & 1u) {
    eveExpressionRequest(EVE_EXPR_AFFECTION, 1800u);
  } else {
    eveExpressionRequest(EVE_EXPR_SOFT_IDLE, 1800u);
  }
#else
  (void)nowMs;
  (void)interacting;
#endif
}

void eveDesktopCompanionInit(void) {
  s_active = false;
  s_charging = false;
  s_lastIdleMs = 0;
  s_lastInteractionMs = 0;
  s_lastAudioMs = 0;
  s_nextIdleMs = 1800u;
  s_lookRight = false;
}

void eveDesktopCompanionSetActive(bool active, bool charging) {
  if (s_active == active && s_charging == charging) {
    return;
  }

  s_active = active;
  s_charging = charging;
  const uint32_t now = millis();

  if (s_active) {
    Serial.println(F("[EVE][DESK] desktop companion active"));
    eyesNotifyDockingState(true, s_charging);
    requestDockExpression_(now, false);
    setGaze_(0.0f, 0.0f);
    servoSetHeadPanTarget(90);
    servoSetRightArmTarget(88);
    neopixelSetPattern(s_charging ? 4u : 1u);
    s_lastIdleMs = now;
    s_nextIdleMs = 1200u;
  } else {
    Serial.println(F("[EVE][DESK] desktop companion inactive"));
    eyesNotifyDockingState(false, false);
    setGaze_(0.0f, 0.0f);
    servoSetHeadPanTarget(90);
    servoSetRightArmTarget(90);
  }
}

bool eveDesktopCompanionIsActive(void) {
  return s_active;
}

static bool readNearInteraction_(uint32_t nowMs, float* gazeX, int16_t* panDeg) {
  if (gazeX) {
    *gazeX = 0.0f;
  }
  if (panDeg) {
    *panDeg = 90;
  }

#if EVE_ENABLE_TOF
  eveTofManagerPoll(nowMs);

  EveTofRawFrame raw;
  if (!eveTofManagerGetLastFrame(&raw)) {
    return false;
  }

  eveTargetTrackerUpdate(&raw, nowMs);
  EveTargetSnapshot snap;
  eveTargetTrackerGetSnapshot(&snap);

  const bool nearEnough = snap.distanceMm > 0 && snap.distanceMm <= 850;
  const bool confident = snap.confidencePct >= 25u;
  if (!nearEnough || !confident) {
    return false;
  }

  if (snap.zone == EVE_TARGET_MODEL_LEFT) {
    if (gazeX) *gazeX = -0.65f;
    if (panDeg) *panDeg = 72;
  } else if (snap.zone == EVE_TARGET_MODEL_RIGHT) {
    if (gazeX) *gazeX = 0.65f;
    if (panDeg) *panDeg = 108;
  } else {
    if (gazeX) *gazeX = 0.0f;
    if (panDeg) *panDeg = 90;
  }
  return true;
#else
  (void)nowMs;
  return false;
#endif
}

static void idleCompanionTick_(uint32_t nowMs) {
  if ((uint32_t)(nowMs - s_lastIdleMs) < s_nextIdleMs) {
    return;
  }

  s_lastIdleMs = nowMs;
  s_nextIdleMs = 1800u + (uint32_t)(rand() % 2600);
  s_lookRight = !s_lookRight;

  const float gx = s_lookRight ? 0.42f : -0.42f;
  const int16_t pan = s_lookRight ? 102 : 78;
  setGaze_(gx, -0.08f);
  servoSetHeadPanTarget(pan);
  servoSetRightArmTarget(s_lookRight ? 96 : 84);
  requestDockExpression_(nowMs, false);
  neopixelSetPattern(s_charging ? 4u : 1u);
}

void eveDesktopCompanionTick(uint32_t nowMs) {
  if (!s_active) {
    return;
  }

  float gazeX = 0.0f;
  int16_t panDeg = 90;
  const bool interacting = readNearInteraction_(nowMs, &gazeX, &panDeg);

  if (interacting) {
    s_lastInteractionMs = nowMs;
    setGaze_(gazeX, -0.18f);
    servoSetHeadPanTarget(panDeg);
    servoSetRightArmTarget(112);
    requestDockExpression_(nowMs, true);
    neopixelSetPattern(5u);

    if ((uint32_t)(nowMs - s_lastAudioMs) > 18000u) {
      s_lastAudioMs = nowMs;
      audioPlayTrack(1);
    }
    return;
  }

  if ((uint32_t)(nowMs - s_lastInteractionMs) < 1800u) {
    return;
  }

  idleCompanionTick_(nowMs);
}
