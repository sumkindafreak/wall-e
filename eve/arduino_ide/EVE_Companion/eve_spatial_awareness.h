#pragma once

#include <Arduino.h>
#include <stdint.h>

#define EVE_SPATIAL_FLAG_SLEEP 0x01
#define EVE_SPATIAL_FLAG_LOW_BATTERY 0x02
#define EVE_SPATIAL_FLAG_ALERT 0x04
#define EVE_SPATIAL_FLAG_CONFUSED 0x08
#define EVE_SPATIAL_FLAG_ATTACHED 0x10

void eveSpatialAwarenessInit(void);
void eveSpatialAwarenessTick(uint32_t nowMs);
void eveSpatialSetBehaviorFlags(uint8_t flags);
