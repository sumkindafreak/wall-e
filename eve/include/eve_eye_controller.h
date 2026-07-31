/**
 * EVE Eye Controller — sole owner of left/right eye rendering.
 * Behaviour → Emotion → Eye Controller → displays. Nothing else draws to the panels.
 */
#pragma once

#include "config.h"

#if EVE_ENABLE_EYES

#include <Arduino.h>
#include "eve_eye.h"
#include "eve_expression_state.h"
#include "eve_gaze_engine.h"

typedef struct {
  EveExpressionId expression;
  EveGazeDirection look;
  bool blinkRequest;
  bool slowBlink;
  bool doubleBlink;
  bool leftWink;
  bool rightWink;
} EveEyeBehaviourRequest;

void eveEyeControllerInit(void);
void eveEyeControllerTick(uint32_t nowMs);

EveEye* eveEyeControllerLeft(void);
EveEye* eveEyeControllerRight(void);

/** Emotion / behaviour layers call this; controller performs rendering. */
void eveEyeControllerApplyRequest(const EveEyeBehaviourRequest* req);

#endif /* EVE_ENABLE_EYES */
