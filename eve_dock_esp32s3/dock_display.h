#pragma once
#include <Arduino.h>
#include "dock_control.h"
void dockDisplayInit(void);
void dockDisplayUpdate(uint32_t now_ms, DockFsmState state, bool chargingEnabled, uint32_t lastEveRxMs);
void dockDisplayUpdateEx(uint32_t now_ms,
                         DockFsmState state,
                         bool chargingEnabled,
                         bool chargeVoltageOk,
                         uint32_t lastEveRxMs,
                         bool chargeSafetyLocked = false,
                         const char* chargeSafetyReason = nullptr);
