/**
 * EVE face — non-repetitive idle micro-behaviours (blinks, glances, pauses).
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "eve_emotion_engine.h"

void eveIdleInit(void);
void eveIdleTick(uint32_t nowMs);

/** Enable/disable idle scheduling (off during sleep / boot). */
void eveIdleSetEnabled(bool on);

/** Emotion transitions reset pacing so timing never feels looped. */
void eveIdleOnEmotionChange(EveEmotionState state);
