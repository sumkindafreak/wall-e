#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include "eve_target_tracker.h"
#include "eve_head_tracking_manager.h"

void eveTargetRelayInit(void);
void eveTargetRelayTick(uint32_t nowMs, const EveTargetSnapshot* snap, bool ackTriggered, EveHeadTrackState headSt,
                        int16_t headPanDeg, uint32_t zoneStableSinceMs);
