#include "eve_spatial_awareness.h"
#include "eve_tof_manager.h"
#include "eve_target_tracker.h"
#include "eve_acknowledgement_manager.h"
#include "eve_head_tracking_manager.h"
#include "eve_target_relay.h"
#include "servo_control.h"
#include "config.h"

static uint8_t s_behaviorFlags = 0;
static EveTargetModel s_prevOut = EVE_TARGET_MODEL_NONE;
static uint32_t s_zoneStableSince = 0;
static uint32_t s_lastZoneLog = 0;

void eveSpatialSetBehaviorFlags(uint8_t flags) {
  s_behaviorFlags = flags;
}

void eveSpatialAwarenessInit(void) {
  s_behaviorFlags = 0;
  s_prevOut = EVE_TARGET_MODEL_NONE;
  s_zoneStableSince = millis();
  s_lastZoneLog = 0;
#if EVE_ENABLE_TOF
  eveTofManagerInit();
  eveTargetTrackerInit();
  eveAcknowledgementInit();
  eveHeadTrackingInit();
  eveTargetRelayInit();
#endif
}

void eveSpatialAwarenessTick(uint32_t nowMs) {
#if !EVE_ENABLE_TOF
  (void)nowMs;
  return;
#endif

  eveTofManagerPoll(nowMs);

  EveTofRawFrame raw;
  eveTofManagerGetLastFrame(&raw);
  eveTargetTrackerUpdate(&raw, nowMs);

  EveTargetSnapshot snap;
  eveTargetTrackerGetSnapshot(&snap);

  if (snap.zone != s_prevOut) {
    s_prevOut = snap.zone;
    s_zoneStableSince = nowMs;
  }

  if (snap.zone == EVE_TARGET_MODEL_LEFT || snap.zone == EVE_TARGET_MODEL_RIGHT ||
      snap.zone == EVE_TARGET_MODEL_CENTER) {
    if (snap.confidencePct >= 28 && (nowMs - s_lastZoneLog) > 650u) {
      s_lastZoneLog = nowMs;
      Serial.printf("[EVE_TOF] %s zone stable, confidence %u\n", snap.zone == EVE_TARGET_MODEL_LEFT
                                                                       ? "Left"
                                                                       : snap.zone == EVE_TARGET_MODEL_RIGHT
                                                                             ? "Right"
                                                                             : "Center",
                    (unsigned)snap.confidencePct);
    }
  }

  bool ack = eveAcknowledgementEvaluate(&snap, nowMs, s_behaviorFlags);

  if (ack) {
    if (snap.zone == EVE_TARGET_MODEL_LEFT) {
      Serial.println(F("[EVE_TOF] Ack roll passed, turning head left"));
    } else if (snap.zone == EVE_TARGET_MODEL_RIGHT) {
      Serial.println(F("[EVE_TOF] Ack roll passed, turning head right"));
    } else if (snap.zone == EVE_TARGET_MODEL_CENTER || snap.zone == EVE_TARGET_MODEL_MULTI) {
      Serial.println(F("[EVE_TOF] Ack roll passed, centering gaze"));
    }
  }

  eveHeadTrackingTick(nowMs, &snap, ack, s_behaviorFlags);
  servoSetHeadPanTarget(eveHeadTrackingGetPanDeg());

  eveTargetRelayTick(nowMs, &snap, ack, eveHeadTrackingGetState(), eveHeadTrackingGetPanDeg(), s_zoneStableSince);
}
