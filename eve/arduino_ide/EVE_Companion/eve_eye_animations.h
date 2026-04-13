/**
 * EVE face — LVGL 9 animation helpers (blink, saccades, glow pulse, wake/sleep).
 */
#pragma once

#include "config.h"

#if EVE_ENABLE_EYES

#include <lvgl.h>
#include "eve_eye_renderer.h"

void eveEyeAnimationsInit(EveEyeUi* ui);
void eveEyeAnimationsTick(uint32_t nowMs, float dtSec);

void eveEyeAnimationsTriggerBlink(void);
void eveEyeAnimationsTriggerDoubleBlink(void);
void eveEyeAnimationsNudgeGaze(float dx, float dy);
void eveEyeAnimationsSetSleepClosed(bool closed);
void eveEyeAnimationsWakeOpen(void);

/** Extra lid closure 0..1 (blink) merged in display tick */
float eveEyeAnimationsBlinkLid(void);

void eveEyeAnimationsGetMicroGaze(float* dx, float* dy);

#endif /* EVE_ENABLE_EYES */
