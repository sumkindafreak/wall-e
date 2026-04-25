#pragma once

#include <Arduino.h>

void eveMoodEffectsInit(void);
void eveMoodEffectsTick(uint32_t nowMs);

/** Called when NeoPixel pattern changes — drives smooth target colour (no protocol impact). */
void eveMoodEffectsOnPattern(uint8_t pattern);
