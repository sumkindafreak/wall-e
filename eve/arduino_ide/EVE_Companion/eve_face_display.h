/**
 * EVE face — LVGL init, display flush, tick glue.
 */
#pragma once

#include <Arduino.h>

void eveFaceDisplayInit(void);
void eveFaceDisplayTick(uint32_t nowMs);
