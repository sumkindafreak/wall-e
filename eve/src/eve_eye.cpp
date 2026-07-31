#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye.h"
#include <math.h>
#include <string.h>

void eveEyeInit(EveEye* eye, EveEyeSide side) {
  if (!eye) {
    return;
  }
  memset(eye, 0, sizeof(*eye));
  eye->side = side;
  eye->smooth.brightness = 1.f;
  eye->target.brightness = 1.f;
  eye->blinkPhaseOffsetMs = (side == EVE_EYE_SIDE_RIGHT) ? (120u + (uint32_t)(rand() % 90)) : 0u;
}

void eveEyeResetSmooth(EveEye* eye) {
  if (!eye) {
    return;
  }
  eye->smooth = eye->target;
}

static float sideGazeBias(EveEyeSide side, float sharedGx) {
  if (side == EVE_EYE_SIDE_LEFT) {
    return sharedGx - 0.04f;
  }
  return sharedGx + 0.04f;
}

void eveEyeApplySharedTarget(EveEye* eye, const EveEyeTarget* shared, float lidLocal, float lidShared) {
  if (!eye || !shared) {
    return;
  }
  eye->target.gazeX = sideGazeBias(eye->side, shared->gazeX) + eye->microGazeX;
  eye->target.gazeY = shared->gazeY + eye->microGazeY;
  eye->target.scaleX = shared->eyeScaleX;
  eye->target.scaleY = shared->eyeScaleY * (1.f - 0.45f * shared->squint);
  eye->target.squint = shared->squint;
  eye->target.glowOpa = shared->glowOpa;
  eye->target.scanOpa = shared->scanOpa;
  eye->target.tiltDeg = shared->tiltDeg;
  eye->target.brightness = 1.f;
  float lid = fminf(1.f, fmaxf(shared->lid, lidShared));
  eye->target.lidUpper = fminf(1.f, fmaxf(lid, lidLocal));
  eye->target.lidLower = shared->squint * 0.15f;
}

static void lerpVis(EveEyeVisual* s, const EveEyeVisual* t, float a) {
#define L(F) s->F += (t->F - s->F) * a
  L(gazeX);
  L(gazeY);
  L(scaleX);
  L(scaleY);
  L(lidUpper);
  L(lidLower);
  L(squint);
  L(glowOpa);
  L(brightness);
  L(scanOpa);
  L(tiltDeg);
#undef L
}

void eveEyeTick(EveEye* eye, float ease) {
  if (!eye) {
    return;
  }
  lerpVis(&eye->smooth, &eye->target, ease);
}

#endif /* EVE_ENABLE_EYES */
