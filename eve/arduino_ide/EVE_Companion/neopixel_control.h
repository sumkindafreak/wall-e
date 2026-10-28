/**
 * EVE glow — onboard RGB LED (GPIO48 heartbeat) and the optional external NeoPixel strip.
 *
 * Layers, highest priority first:
 *   1. STOPPED  (latched stop)          — steady dim red
 *   2. FAULT    (blocking fault)        — red double blink
 *   3. overlay  (expression routine)    — for the routine's duration
 *   4. operator (SET_GLOW / web / CLI)  — until the next mode change ("auto" releases it)
 *   5. base     (from EVE's mode)
 * Pattern numbers: EveGlowPattern in eve_output_ids.h (0,1,3,4,5 keep their old meaning).
 * The external strip stays off unless EVE_ENABLE_NEOPIXEL = 1 and EVE_NEOPIXEL_PIN is set.
 */
#pragma once

#include <Arduino.h>

void neopixelInit(void);
void neopixelTick(void);

/** Legacy entry point (web UI, desktop companion, CLI): same as neopixelSetOperator. */
void neopixelSetPattern(uint8_t pattern);
/** Operator pattern; -1 = automatic (follow the mode). */
void neopixelSetOperator(int8_t pattern);
/** Base pattern chosen from EVE's mode. */
void neopixelSetBase(uint8_t pattern);
/** Routine overlay for ms (0 = until released); pattern 0xFF releases it. */
void neopixelSetOverlay(uint8_t pattern, uint32_t ms);

/** Pattern actually shown right now. */
uint8_t neopixelCurrentPattern(void);
bool neopixelStripActive(void);
bool neopixelOnboardActive(void);
