#include "eve_head_tracking_manager.h"
#include "config.h"
#include "eve_spatial_awareness.h"
#include <math.h>
#include <stdlib.h>

#if EVE_ENABLE_EYES
#include "eve_expression_state.h"
#endif

#ifndef EVE_HEAD_PAN_CENTER
#define EVE_HEAD_PAN_CENTER 90
#endif
#ifndef EVE_HEAD_PAN_LEFT
#define EVE_HEAD_PAN_LEFT 58
#endif
#ifndef EVE_HEAD_PAN_RIGHT
#define EVE_HEAD_PAN_RIGHT 122
#endif

static EveHeadTrackState s_st = TRACK_IDLE;
static int16_t s_pan = EVE_HEAD_PAN_CENTER;
static int16_t s_panTgt = EVE_HEAD_PAN_CENTER;
static uint32_t s_followUntil = 0;
static uint32_t s_returnUntil = 0;
static uint32_t s_holdUntil = 0;

void eveHeadTrackingInit(void) {
  s_st = TRACK_IDLE;
  s_pan = s_panTgt = EVE_HEAD_PAN_CENTER;
  s_followUntil = s_returnUntil = s_holdUntil = 0;
}

EveHeadTrackState eveHeadTrackingGetState(void) {
  return s_st;
}

int16_t eveHeadTrackingGetPanDeg(void) {
  return s_pan;
}

static int16_t panForZone(EveTargetModel z) {
  switch (z) {
    case EVE_TARGET_MODEL_LEFT:
      return EVE_HEAD_PAN_LEFT;
    case EVE_TARGET_MODEL_RIGHT:
      return EVE_HEAD_PAN_RIGHT;
    case EVE_TARGET_MODEL_CENTER:
    case EVE_TARGET_MODEL_MULTI:
      return EVE_HEAD_PAN_CENTER;
    default:
      return EVE_HEAD_PAN_CENTER;
  }
}

static void syncEyesFromPan(void) {
#if EVE_ENABLE_EYES
  float nx = 0.5f + (float)(s_pan - EVE_HEAD_PAN_CENTER) / 70.f;
  float ny = 0.48f;
  if (nx < 0.f) {
    nx = 0.f;
  }
  if (nx > 1.f) {
    nx = 1.f;
  }
  eveExpressionSetTargetGaze(nx, ny);
  bool tr = (s_st == TRACK_FOLLOW_LEFT || s_st == TRACK_FOLLOW_RIGHT || s_st == TRACK_FOLLOW_CENTER ||
             s_st == TRACK_ACK_LEFT || s_st == TRACK_ACK_RIGHT || s_st == TRACK_ACK_CENTER);
  eveExpressionSetTracking(tr);
#endif
}

void eveHeadTrackingTick(uint32_t nowMs, const EveTargetSnapshot* snap, bool ackThisFrame, uint8_t spatialFlags) {
  if (spatialFlags & EVE_SPATIAL_FLAG_SLEEP) {
    s_st = TRACK_RETURN_HOME;
    s_panTgt = EVE_HEAD_PAN_CENTER;
  }

  if (ackThisFrame && snap && snap->zone != EVE_TARGET_MODEL_NONE && snap->zone != EVE_TARGET_MODEL_UNCERTAIN) {
    s_panTgt = panForZone(snap->zone);
    if (snap->zone == EVE_TARGET_MODEL_LEFT) {
      s_st = TRACK_ACK_LEFT;
    } else if (snap->zone == EVE_TARGET_MODEL_RIGHT) {
      s_st = TRACK_ACK_RIGHT;
    } else {
      s_st = TRACK_ACK_CENTER;
    }
    s_holdUntil = nowMs + 420u + (uint32_t)(rand() % 380);
    uint32_t fv = 1400u + (uint32_t)(rand() % 900);
    s_followUntil = s_holdUntil + fv;
    Serial.print(F("[EVE_TOF] Ack: head toward zone, pan="));
    Serial.println((int)s_panTgt);
  }

  if (s_st != TRACK_IDLE && s_st != TRACK_RETURN_HOME && snap) {
    if (snap->zone == EVE_TARGET_MODEL_NONE || snap->confidencePct < 12) {
      s_st = TRACK_RETURN_HOME;
      s_panTgt = EVE_HEAD_PAN_CENTER;
      s_returnUntil = nowMs + 500u + (uint32_t)(rand() % 400);
    } else if (nowMs > s_followUntil) {
      s_st = TRACK_RETURN_HOME;
      s_panTgt = EVE_HEAD_PAN_CENTER;
      s_returnUntil = nowMs + 600u;
    } else if (nowMs > s_holdUntil) {
      if (snap->zone == EVE_TARGET_MODEL_LEFT) {
        s_st = TRACK_FOLLOW_LEFT;
      } else if (snap->zone == EVE_TARGET_MODEL_RIGHT) {
        s_st = TRACK_FOLLOW_RIGHT;
      } else if (snap->zone == EVE_TARGET_MODEL_CENTER || snap->zone == EVE_TARGET_MODEL_MULTI) {
        s_st = TRACK_FOLLOW_CENTER;
      }
      s_panTgt = panForZone(snap->zone);
      int16_t j = (int16_t)(rand() % 9 - 4);
      s_panTgt = (int16_t)constrain((int)s_panTgt + j, 45, 135);
    }
  }

  if (s_st == TRACK_RETURN_HOME) {
    s_panTgt = EVE_HEAD_PAN_CENTER;
    if (nowMs > s_returnUntil && abs((int)s_pan - EVE_HEAD_PAN_CENTER) < 4) {
      s_st = TRACK_IDLE;
    }
  }

  float speed = 0.11f;
  if (spatialFlags & EVE_SPATIAL_FLAG_LOW_BATTERY) {
    speed = 0.07f;
  }
  speed += (float)(rand() % 5) * 0.004f;
  s_pan += (int16_t)((float)(s_panTgt - s_pan) * speed);
  s_pan = (int16_t)constrain((int)s_pan, 45, 135);

  syncEyesFromPan();

  if ((s_st == TRACK_FOLLOW_LEFT || s_st == TRACK_FOLLOW_RIGHT || s_st == TRACK_FOLLOW_CENTER) &&
      snap && snap->distanceMm > 0) {
    static uint32_t s_lastLog;
    if (nowMs - s_lastLog > 500u) {
      s_lastLog = nowMs;
      Serial.printf("[EVE_TOF] Tracking active, distance %ld mm\n", (long)snap->distanceMm);
    }
  }
}
