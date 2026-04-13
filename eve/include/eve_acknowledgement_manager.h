#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "eve_target_tracker.h"

void eveAcknowledgementInit(void);
/** Returns true when this frame should trigger a visible acknowledgement (head + relay flag). */
bool eveAcknowledgementEvaluate(const EveTargetSnapshot* snap, uint32_t nowMs, uint8_t spatialFlags);
