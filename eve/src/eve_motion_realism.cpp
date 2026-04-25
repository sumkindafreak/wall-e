#include "eve_motion_realism.h"

#include <stdlib.h>

#include <cmath>

float eveEaseInOut(float t) {
  if (t <= 0.f) {
    return 0.f;
  }
  if (t >= 1.f) {
    return 1.f;
  }
  return t * t * (3.f - 2.f * t);
}

static uint32_t s_lastServoTickMs = 0;
static uint32_t s_lastBusyMs = 0;
static int16_t s_lastAutoDeg = 90;

void eveMotionRealismInit(void) {
  const uint32_t now = millis();
  s_lastServoTickMs = now;
  s_lastBusyMs = now;
  s_lastAutoDeg = 90;
}

uint32_t eveMotionRealismServoDeltaMs(void) {
  const uint32_t now = millis();
  uint32_t dt = now - s_lastServoTickMs;
  s_lastServoTickMs = now;
  if (dt > 48u) {
    dt = 48u;
  }
  if (dt < 1u) {
    dt = 1u;
  }
  return dt;
}

int16_t eveMotionRealismApplyRemoteHeadDeg(int16_t deg, uint32_t nowMs) {
  s_lastBusyMs = nowMs;
  const int base = constrain((int)deg, 45, 135);
  const int jitter = (rand() % 7) - 3;
  return (int16_t)constrain(base + jitter, 45, 135);
}

void eveMotionRealismNotifyAutonomousHeadTarget(int16_t deg, uint32_t nowMs) {
  const int d = (int)deg;
  if (abs(d - (int)s_lastAutoDeg) > 1) {
    s_lastBusyMs = nowMs;
  }
  s_lastAutoDeg = deg;
}

int16_t eveMotionRealismStepToward(int16_t current, int16_t target, uint32_t dtMs, bool headChannel) {
  int32_t diff = (int32_t)target - (int32_t)current;
  if (diff == 0) {
    return current;
  }

  const float spanMs = headChannel ? 200.f : 260.f;
  float u = eveEaseInOut((float)dtMs / spanMs);
  if (u < 0.04f) {
    u = 0.04f;
  }
  if (u > 1.f) {
    u = 1.f;
  }

  const float vary = 0.90f + 0.22f * ((float)(rand() % 101) / 100.f);
  int32_t step = (int32_t)lroundf((float)diff * u * vary);
  if (step == 0) {
    step = (diff > 0) ? 1 : -1;
  }

  int32_t next = (int32_t)current + step;
  if ((diff > 0 && next > target) || (diff < 0 && next < target)) {
    next = target;
  }
  return (int16_t)next;
}

int16_t eveMotionRealismStepArmToward(int16_t current, int16_t target, uint32_t dtMs) {
  int32_t diff = (int32_t)target - (int32_t)current;
  if (diff == 0) {
    return current;
  }

  const float spanMs = 320.f;
  float u = eveEaseInOut((float)dtMs / spanMs) * 0.52f;
  if (u < 0.02f) {
    u = 0.02f;
  }
  if (u > 1.f) {
    u = 1.f;
  }

  const float vary = 0.88f + 0.20f * ((float)(rand() % 101) / 100.f);
  int32_t step = (int32_t)lroundf((float)diff * u * vary);
  if (step == 0) {
    step = (diff > 0) ? 1 : -1;
  }

  int32_t next = (int32_t)current + step;
  if ((diff > 0 && next > target) || (diff < 0 && next < target)) {
    next = target;
  }
  return (int16_t)next;
}

void eveMotionRealismIdleTick(int16_t* headDesiredInOut, uint32_t nowMs, bool headSettled) {
  if (!headDesiredInOut) {
    return;
  }
  if ((uint32_t)(nowMs - s_lastBusyMs) < 3200u) {
    return;
  }
  if (!headSettled) {
    return;
  }

  static uint32_t s_phaseStartMs = 0;
  static int8_t s_drift = 0;
  if (s_phaseStartMs == 0u) {
    s_phaseStartMs = nowMs;
  }

  const uint32_t phaseDur = 5200u + (uint32_t)(rand() % 3800);
  if ((uint32_t)(nowMs - s_phaseStartMs) > phaseDur) {
    s_phaseStartMs = nowMs;
    s_drift = (int8_t)((rand() % 5) - 2);
    return;
  }

  int v = (int)*headDesiredInOut + (int)s_drift / 3;
  if (s_drift != 0 && (rand() % 80) == 0) {
    v += (s_drift > 0) ? 1 : -1;
  }
  *headDesiredInOut = (int16_t)constrain(v, 45, 135);
}
