#pragma once

#include <Arduino.h>

void systemStatusInit(void);
void systemStatusTick(void);
uint32_t systemStatusUptimeMs(void);
