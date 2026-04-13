#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include "eve_target_tracker.h"

typedef enum {
  TRACK_IDLE = 0,
  TRACK_ACK_LEFT,
  TRACK_ACK_RIGHT,
  TRACK_ACK_CENTER,
  TRACK_FOLLOW_LEFT,
  TRACK_FOLLOW_CENTER,
  TRACK_FOLLOW_RIGHT,
  TRACK_RETURN_HOME,
} EveHeadTrackState;

void eveHeadTrackingInit(void);
void eveHeadTrackingTick(uint32_t nowMs, const EveTargetSnapshot* snap, bool ackThisFrame, uint8_t spatialFlags);

EveHeadTrackState eveHeadTrackingGetState(void);
int16_t eveHeadTrackingGetPanDeg(void);
