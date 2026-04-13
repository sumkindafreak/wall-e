#include "eve_target_tracker.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static float s_fl = 9999.f;
static float s_fr = 9999.f;
static float s_fc = 9999.f;

static EveTargetModel s_candidate = EVE_TARGET_MODEL_NONE;
static uint8_t s_agree = 0;
static EveTargetModel s_out = EVE_TARGET_MODEL_NONE;
static uint8_t s_conf = 0;
static uint32_t s_outSince = 0;

static bool nearMm(int32_t mm) {
  return mm > 0 && mm <= EVE_TOF_NEAR_MM;
}

static EveTargetModel classifySmoothed(float fl, float fr, float fc) {
  bool vl = nearMm((int32_t)fl);
  bool vr = nearMm((int32_t)fr);
  bool vc = nearMm((int32_t)fc);

  if (vc && !vl && !vr) {
    return EVE_TARGET_MODEL_CENTER;
  }
  if (!vl && !vr && !vc) {
    return EVE_TARGET_MODEL_NONE;
  }
  if (vl && vr) {
    int32_t dl = (int32_t)fl;
    int32_t dr = (int32_t)fr;
    int diff = abs((int)(dl - dr));
    if (diff < 120) {
      return EVE_TARGET_MODEL_MULTI;
    }
    return (dl < dr) ? EVE_TARGET_MODEL_LEFT : EVE_TARGET_MODEL_RIGHT;
  }
  if (vl && !vr) {
    return EVE_TARGET_MODEL_LEFT;
  }
  if (vr && !vl) {
    return EVE_TARGET_MODEL_RIGHT;
  }
  if (vc && (vl || vr)) {
    return EVE_TARGET_MODEL_UNCERTAIN;
  }
  return EVE_TARGET_MODEL_NONE;
}

static int32_t pickDistance(EveTargetModel z) {
  switch (z) {
    case EVE_TARGET_MODEL_LEFT:
      return nearMm((int32_t)s_fl) ? (int32_t)s_fl : -1;
    case EVE_TARGET_MODEL_RIGHT:
      return nearMm((int32_t)s_fr) ? (int32_t)s_fr : -1;
    case EVE_TARGET_MODEL_CENTER:
      return nearMm((int32_t)s_fc) ? (int32_t)s_fc : -1;
    case EVE_TARGET_MODEL_MULTI:
      if (nearMm((int32_t)s_fl) && nearMm((int32_t)s_fr)) {
        return ((int32_t)s_fl + (int32_t)s_fr) / 2;
      }
      return -1;
    default:
      return -1;
  }
}

void eveTargetTrackerInit(void) {
  s_fl = s_fr = s_fc = 9999.f;
  s_candidate = EVE_TARGET_MODEL_NONE;
  s_agree = 0;
  s_out = EVE_TARGET_MODEL_NONE;
  s_conf = 0;
  s_outSince = 0;
}

void eveTargetTrackerUpdate(const EveTofRawFrame* raw, uint32_t nowMs) {
  (void)nowMs;
  if (!raw) {
    return;
  }

  const float a = 0.38f;
  if (raw->valid_mask & 1u) {
    s_fl = s_fl * (1.f - a) + (float)raw->left_mm * a;
  } else {
    s_fl = fminf(9999.f, s_fl + 95.f);
  }
  if (raw->valid_mask & 2u) {
    s_fr = s_fr * (1.f - a) + (float)raw->right_mm * a;
  } else {
    s_fr = fminf(9999.f, s_fr + 95.f);
  }
  if (raw->valid_mask & 4u) {
    s_fc = s_fc * (1.f - a) + (float)raw->center_mm * a;
  } else {
    s_fc = fminf(9999.f, s_fc + 95.f);
  }

  EveTargetModel z = classifySmoothed(s_fl, s_fr, s_fc);

  if (z == s_candidate) {
    if (s_agree < 255) {
      s_agree++;
    }
  } else {
    s_candidate = z;
    s_agree = 1;
  }

  if (s_agree >= 3) {
    if (z != s_out) {
      s_out = z;
      s_outSince = millis();
      s_conf = (uint8_t)min(100, (int)s_conf + 25);
    } else {
      if (z != EVE_TARGET_MODEL_NONE && s_conf < 100) {
        s_conf++;
      }
    }
  }

  if (z == EVE_TARGET_MODEL_NONE) {
    s_conf = (uint8_t)max(0, (int)s_conf - 3);
    if (s_conf < 8) {
      s_out = EVE_TARGET_MODEL_NONE;
    }
  }
}

void eveTargetTrackerGetSnapshot(EveTargetSnapshot* out) {
  if (!out) {
    return;
  }
  memset(out, 0, sizeof(*out));
  out->zone = s_out;
  out->confidencePct = s_conf;
  out->distanceMm = pickDistance(s_out);
  out->stableStrong = (millis() - s_outSince) > 700u && s_conf > 50 && s_out != EVE_TARGET_MODEL_NONE;
}
