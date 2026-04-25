#pragma once
#include <Arduino.h>
#include "dock_control.h"
void dockDisplayInit(void);
void dockDisplayUpdate(uint32_t now_ms, DockFsmState state, bool chargingEnabled, uint32_t lastEveRxMs);
