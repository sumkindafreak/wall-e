/**
 * EVE — high-level emotion state machine coordinating expression, gaze, idle, ToF, voice.
 * Eyes-only behaviour: no mouth / lip sync / jaw animation.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "eve_target_tracker.h"

typedef enum {
  EVE_EMOTION_BOOT = 0,
  EVE_EMOTION_IDLE,
  EVE_EMOTION_CURIOUS,
  EVE_EMOTION_FOLLOW,
  EVE_EMOTION_INTERACT,
  EVE_EMOTION_HAPPY,
  EVE_EMOTION_THINKING,
  EVE_EMOTION_SLEEP,
} EveEmotionState;

void eveEmotionInit(void);
void eveEmotionTick(uint32_t nowMs);

EveEmotionState eveEmotionGetState(void);
const char* eveEmotionStateName(EveEmotionState st);

void eveEmotionNotifyBootComplete(void);
void eveEmotionNotifySleep(bool sleeping);
void eveEmotionNotifyVoice(bool playing, uint8_t track);

/** Called from spatial pipeline after target tracker update. */
void eveEmotionOnTofSnapshot(const EveTargetSnapshot* snap, uint32_t nowMs);

/** Optional: force a thinking beat (UART / desktop companion). */
void eveEmotionRequestThinking(uint32_t holdMs);
