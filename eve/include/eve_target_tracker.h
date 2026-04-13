#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "eve_tof_manager.h"

typedef enum {
  EVE_TARGET_MODEL_NONE = 0,
  EVE_TARGET_MODEL_LEFT,
  EVE_TARGET_MODEL_CENTER,
  EVE_TARGET_MODEL_RIGHT,
  EVE_TARGET_MODEL_MULTI,
  EVE_TARGET_MODEL_UNCERTAIN,
} EveTargetModel;

typedef struct {
  EveTargetModel zone;
  uint8_t confidencePct;
  int32_t distanceMm;
  bool stableStrong;
} EveTargetSnapshot;

void eveTargetTrackerInit(void);
void eveTargetTrackerUpdate(const EveTofRawFrame* raw, uint32_t nowMs);
void eveTargetTrackerGetSnapshot(EveTargetSnapshot* out);
