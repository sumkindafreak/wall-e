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
void eveEyeAnimationsTriggerSlowBlink(void);
/** Alias for slow blink (long lid hold). */
#define eveEyeAnimationsTriggerLongBlink eveEyeAnimationsTriggerSlowBlink
/** Optional playful wink; leftEye=true closes left lid only. */
void eveEyeAnimationsTriggerWink(bool leftEye);
void eveEyeAnimationsTriggerSquint(float amount);
void eveEyeAnimationsTriggerWiden(float amount);
void eveEyeAnimationsNudgeGaze(float dx, float dy);
void eveEyeAnimationsSetSleepClosed(bool closed);
void eveEyeAnimationsWakeOpen(void);

/** Per-eye lid closure 0..1 (blink / wink) merged in display tick */
void eveEyeAnimationsGetLids(float* left, float* right);

void eveEyeAnimationsGetMicroGaze(float* dx, float* dy);

/** Transient squint/widen overlay 0..1 decaying each tick */
float eveEyeAnimationsSquintOverlay(void);
float eveEyeAnimationsWidenOverlay(void);

#endif /* EVE_ENABLE_EYES */
