#pragma once

#include <Arduino.h>

void eveDesktopCompanionInit(void);
void eveDesktopCompanionSetActive(bool active, bool charging);
void eveDesktopCompanionTick(uint32_t nowMs);
bool eveDesktopCompanionIsActive(void);
bool eveDesktopCompanionApplyConfigJson(const char* json);
