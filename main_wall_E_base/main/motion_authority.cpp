#include "motion_authority.h"
#include <Arduino.h>
#include <Preferences.h>

static const char* kNs = "walle_ma";
static const char* kKey = "mode";
static MotionAuthorityMode s_mode = MOTION_AUTH_ANY;

void motionAuthorityInit(void) {
  Preferences p;
  p.begin(kNs, true);
  uint8_t v = p.getUChar(kKey, 0);
  p.end();
  if (v > MOTION_AUTH_WEB_ONLY) v = MOTION_AUTH_ANY;
  s_mode = (MotionAuthorityMode)v;
  Serial.printf("[MotionAuth] mode=%s\n", motionAuthorityModeName(s_mode));
}

void motionAuthoritySet(MotionAuthorityMode m) {
  if (m > MOTION_AUTH_WEB_ONLY) m = MOTION_AUTH_ANY;
  s_mode = m;
  Preferences p;
  p.begin(kNs, false);
  p.putUChar(kKey, (uint8_t)m);
  p.end();
}

MotionAuthorityMode motionAuthorityGet(void) {
  return s_mode;
}

bool motionAuthorityAllowCyd(void) {
  return s_mode == MOTION_AUTH_ANY || s_mode == MOTION_AUTH_CYD_ONLY;
}

bool motionAuthorityAllowWeb(void) {
  return s_mode == MOTION_AUTH_ANY || s_mode == MOTION_AUTH_WEB_ONLY;
}

const char* motionAuthorityModeName(MotionAuthorityMode m) {
  switch (m) {
    case MOTION_AUTH_ANY: return "any";
    case MOTION_AUTH_CYD_ONLY: return "cyd_only";
    case MOTION_AUTH_WEB_ONLY: return "web_only";
    default: return "any";
  }
}
