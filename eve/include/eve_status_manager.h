#pragma once

#include <Arduino.h>

void eveStatusManagerInit(void);
void eveStatusManagerTick(void);

String eveStatusManagerGetJSON(void);
