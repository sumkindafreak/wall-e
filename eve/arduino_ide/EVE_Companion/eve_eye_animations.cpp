#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye_animations.h"
#include "eve_eye_blink.h"

void eveEyeAnimationsInit(EveEyeUi* ui) {
  (void)ui;
  eveEyeBlinkInit();
}

void eveEyeAnimationsGetLids(float* left, float* right) {
  if (left) {
    *left = eveEyeBlinkLid(EVE_EYE_SIDE_LEFT);
  }
  if (right) {
    *right = eveEyeBlinkLid(EVE_EYE_SIDE_RIGHT);
  }
}

float eveEyeAnimationsSquintOverlay(void) {
  return eveEyeBlinkSquintOverlay();
}

float eveEyeAnimationsWidenOverlay(void) {
  return eveEyeBlinkWidenOverlay();
}

void eveEyeAnimationsTriggerBlink(void) {
  eveEyeBlinkTriggerRandom();
}

void eveEyeAnimationsTriggerDoubleBlink(void) {
  eveEyeBlinkTriggerDouble(EVE_EYE_SIDE_LEFT);
  eveEyeBlinkTriggerDouble(EVE_EYE_SIDE_RIGHT);
}

void eveEyeAnimationsTriggerSlowBlink(void) {
  eveEyeBlinkTriggerSlow(EVE_EYE_SIDE_LEFT);
  eveEyeBlinkTriggerSlow(EVE_EYE_SIDE_RIGHT);
}

void eveEyeAnimationsTriggerWink(bool leftEye) {
  eveEyeBlinkTriggerWink(leftEye ? EVE_EYE_SIDE_LEFT : EVE_EYE_SIDE_RIGHT);
}

void eveEyeAnimationsTriggerSquint(float amount) {
  eveEyeBlinkTriggerSquint(amount);
}

void eveEyeAnimationsTriggerWiden(float amount) {
  eveEyeBlinkTriggerWiden(amount);
}

void eveEyeAnimationsNudgeGaze(float dx, float dy) {
  eveEyeBlinkNudgeGaze(dx, dy);
}

void eveEyeAnimationsSetSleepClosed(bool closed) {
  eveEyeBlinkSetSleepClosed(closed);
}

void eveEyeAnimationsWakeOpen(void) {
  eveEyeBlinkWakeOpen();
}

void eveEyeAnimationsTick(uint32_t nowMs, float dtSec) {
  (void)nowMs;
  (void)dtSec;
}

void eveEyeAnimationsGetMicroGaze(float* dx, float* dy) {
  eveEyeBlinkGetMicroGaze(dx, dy);
}

#endif /* EVE_ENABLE_EYES */
