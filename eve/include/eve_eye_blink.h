/**
 * EVE — independent per-eye blink timing (offsets, winks, slow/double).
 */
#pragma once

#include "config.h"

#if EVE_ENABLE_EYES

#include <Arduino.h>
#include "eve_eye.h"

void eveEyeBlinkInit(void);
void eveEyeBlinkTick(uint32_t nowMs, float dtSec);

float eveEyeBlinkLid(EveEyeSide side);
void eveEyeBlinkTrigger(EveEyeSide side);
void eveEyeBlinkTriggerSlow(EveEyeSide side);
void eveEyeBlinkTriggerDouble(EveEyeSide side);
void eveEyeBlinkTriggerWink(EveEyeSide side);
void eveEyeBlinkTriggerRandom(void);

float eveEyeBlinkSquintOverlay(void);
float eveEyeBlinkWidenOverlay(void);
void eveEyeBlinkTriggerSquint(float amount);
void eveEyeBlinkTriggerWiden(float amount);

void eveEyeBlinkGetMicroGaze(float* dx, float* dy);
void eveEyeBlinkNudgeGaze(float dx, float dy);
void eveEyeBlinkSetSleepClosed(bool closed);
void eveEyeBlinkWakeOpen(void);

#endif /* EVE_ENABLE_EYES */
