#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "relationship_protocol.h"

void relationshipInit(void);
void relationshipTick(uint32_t nowMillis);

uint8_t relationshipGetBondStrength(void);
const WalleBondState* relationshipGetState(void);

void relationshipOnDockSharedEvent(void);
