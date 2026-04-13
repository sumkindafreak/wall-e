#include "eve_target_relay.h"
#include "config.h"
#include "eve_protocol.h"
#include "eve_target_protocol.h"
#include "uart_link.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>

static uint32_t s_lastSendMs;
static EveTargetModel s_lastZone;

static const char* zoneStr(EveTargetModel z) {
  switch (z) {
    case EVE_TARGET_MODEL_LEFT:
      return EVE_TARGET_ZONE_LEFT;
    case EVE_TARGET_MODEL_RIGHT:
      return EVE_TARGET_ZONE_RIGHT;
    case EVE_TARGET_MODEL_CENTER:
      return EVE_TARGET_ZONE_CENTER;
    case EVE_TARGET_MODEL_MULTI:
      return EVE_TARGET_ZONE_MULTI;
    case EVE_TARGET_MODEL_UNCERTAIN:
      return EVE_TARGET_ZONE_UNCERTAIN;
    default:
      return EVE_TARGET_ZONE_NONE;
  }
}

static float biasFor(EveTargetModel z) {
  switch (z) {
    case EVE_TARGET_MODEL_LEFT:
      return -0.18f;
    case EVE_TARGET_MODEL_RIGHT:
      return 0.18f;
    case EVE_TARGET_MODEL_CENTER:
      return 0.0f;
    case EVE_TARGET_MODEL_MULTI:
      return 0.0f;
    default:
      return 0.0f;
  }
}

static bool trackingActive(EveHeadTrackState st) {
  return st == TRACK_FOLLOW_LEFT || st == TRACK_FOLLOW_RIGHT || st == TRACK_FOLLOW_CENTER;
}

void eveTargetRelayInit(void) {
  s_lastSendMs = 0;
  s_lastZone = EVE_TARGET_MODEL_NONE;
}

void eveTargetRelayTick(uint32_t nowMs, const EveTargetSnapshot* snap, bool ackTriggered, EveHeadTrackState headSt,
                        int16_t headPanDeg, uint32_t zoneStableSinceMs) {
  if (!snap) {
    return;
  }

  bool zoneChange = (snap->zone != s_lastZone);
  s_lastZone = snap->zone;

  bool tracking = trackingActive(headSt);
  uint32_t minGap = 180u;
  if (ackTriggered || zoneChange) {
    minGap = 40u;
  }
  if (tracking && (nowMs - s_lastSendMs) < 120u) {
    minGap = 120u;
  }

  if ((nowMs - s_lastSendMs) < minGap && !ackTriggered && !zoneChange) {
    return;
  }

  if (snap->zone == EVE_TARGET_MODEL_NONE && !tracking && !ackTriggered) {
    if (!zoneChange && (nowMs - s_lastSendMs) < 800u) {
      return;
    }
  }

  s_lastSendMs = nowMs;

  uint32_t age = (zoneStableSinceMs > 0 && nowMs >= zoneStableSinceMs) ? (nowMs - zoneStableSinceMs) : 0;

  StaticJsonDocument<384> doc;
  doc[EVE_TARGET_JSON_PROTO] = EVE_TARGET_PROTOCOL_VERSION;
  doc[EVE_TARGET_JSON_ZONE] = zoneStr(snap->zone);
  doc[EVE_TARGET_JSON_CONFIDENCE] = snap->confidencePct;
  doc[EVE_TARGET_JSON_DISTANCE_MM] = snap->distanceMm > 0 ? snap->distanceMm : 0;
  doc[EVE_TARGET_JSON_ACK] = ackTriggered;
  doc[EVE_TARGET_JSON_TRACKING] = tracking;
  doc[EVE_TARGET_JSON_BIAS] = biasFor(snap->zone);
  doc[EVE_TARGET_JSON_NODE] = "eve-s3";
  doc[EVE_TARGET_JSON_AGE_MS] = age;
  doc[EVE_TARGET_JSON_HEAD_PAN] = headPanDeg;
  doc[EVE_TARGET_JSON_NEAR_MM] = EVE_TOF_NEAR_MM;

  String out;
  serializeJson(doc, out);
  uartLinkSendJson(MSG_EVE_TARGET_AWARENESS, out.c_str());
}
