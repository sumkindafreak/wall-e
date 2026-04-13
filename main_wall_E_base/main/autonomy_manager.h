#pragma once

#include <Arduino.h>
#include <stdint.h>

typedef enum {
  BEH_IDLE = 0,
  BEH_ATTENTION,
  BEH_CURIOUS,
  BEH_COMPANION,
  BEH_ALERT,
  BEH_LOW_BATTERY,
  BEH_DOCKING,
  BEH_SLEEP,
  BEH_SEARCHING_FOR_EVE,
  BEH_CELEBRATION,
} WalleBehaviorState;

void autonomyManagerInit(void);
void autonomyManagerTick(uint32_t nowMillis);

WalleBehaviorState autonomyManagerGetState(void);
const char* autonomyManagerGetStateName(void);
