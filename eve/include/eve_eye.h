/**
 * EVE — one biological eye (left or right). Rendering owned by Eye Controller only.
 */
#pragma once

#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_expression_state.h"
#include <stdint.h>

typedef enum {
  EVE_EYE_SIDE_LEFT = 0,
  EVE_EYE_SIDE_RIGHT,
} EveEyeSide;

/** Per-eye render state (layer composition inputs). */
typedef struct {
  float gazeX;
  float gazeY;
  float scaleX;
  float scaleY;
  float lidUpper;
  float lidLower;
  float squint;
  float glowOpa;
  float brightness;
  float scanOpa;
  float tiltDeg;
} EveEyeVisual;

typedef struct {
  EveEyeSide side;
  EveEyeVisual smooth;
  EveEyeVisual target;
  float microGazeX;
  float microGazeY;
  uint32_t blinkPhaseOffsetMs;
} EveEye;

void eveEyeInit(EveEye* eye, EveEyeSide side);
void eveEyeResetSmooth(EveEye* eye);

/** Merge shared expression target into this eye (applies side-specific gaze offset). */
void eveEyeApplySharedTarget(EveEye* eye, const EveEyeTarget* shared, float lidLocal, float lidShared);

/** Smooth toward target; call once per frame. */
void eveEyeTick(EveEye* eye, float ease);

#endif /* EVE_ENABLE_EYES */
