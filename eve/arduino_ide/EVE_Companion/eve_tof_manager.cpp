#include "eve_tof_manager.h"
#include "config.h"
#include <string.h>

#if EVE_ENABLE_TOF && (EVE_I2C_SDA >= 0) && defined(__has_include)
#if __has_include("VL53L1X.h")
#include <VL53L1X.h>
#define EVE_TOF_HAVE_VL53 1
#endif
#endif

#ifndef EVE_TOF_HAVE_VL53
#define EVE_TOF_HAVE_VL53 0
#endif

#if EVE_TOF_HAVE_VL53
#include <Wire.h>
#endif

static EveTofRawFrame s_last;
static uint32_t s_lastPollMs;

#if EVE_TOF_HAVE_VL53
static VL53L1X s_vl_left;
static VL53L1X s_vl_right;
static bool s_hw_dual;
static bool s_hw_single;
#endif

#if EVE_TOF_SIMULATE && EVE_ENABLE_TOF
static uint32_t s_simT0;
#endif

static void setInvalid(EveTofRawFrame* f) {
  memset(f, 0, sizeof(*f));
  f->left_mm = f->right_mm = f->center_mm = 9999;
}

void eveTofManagerInit(void) {
  memset(&s_last, 0, sizeof(s_last));
  s_lastPollMs = 0;
  setInvalid(&s_last);

#if !EVE_ENABLE_TOF
  Serial.println(F("[EVE_TOF] Manager: disabled (EVE_ENABLE_TOF)"));
  return;
#endif

#if EVE_I2C_SDA < 0
  Serial.println(F("[EVE_TOF] Manager: no I2C pins — raw layer idle"));
  return;
#endif

#if EVE_TOF_SIMULATE
  s_simT0 = millis();
  Serial.println(F("[EVE_TOF] Manager: SIMULATE mode"));
  return;
#endif

#if EVE_TOF_HAVE_VL53
  Wire.begin(EVE_I2C_SDA, EVE_I2C_SCL);
  Wire.setClock(400000);

  s_hw_dual = false;
  s_hw_single = false;
  s_vl_left.setBus(&Wire);
  s_vl_right.setBus(&Wire);

#if EVE_TOF_XSHUT_SECOND >= 0
  pinMode(EVE_TOF_XSHUT_SECOND, OUTPUT);
  digitalWrite(EVE_TOF_XSHUT_SECOND, LOW);
  delay(50);
  if (!s_vl_left.init()) {
    Serial.println(F("[EVE_TOF] First VL53L1X init failed"));
    return;
  }
  s_vl_left.setAddress(EVE_TOF_ADDR_FIRST);
  s_vl_left.setDistanceMode(VL53L1X::Long);
  s_vl_left.setMeasurementTimingBudget(35000);
  s_vl_left.startContinuous(33);
  digitalWrite(EVE_TOF_XSHUT_SECOND, HIGH);
  delay(80);
  if (!s_vl_right.init()) {
    Serial.println(F("[EVE_TOF] Second VL53L1X init failed"));
    s_hw_single = true;
    Serial.println(F("[EVE_TOF] Falling back to single-sensor mode (first VL53L1X)"));
    return;
  }
  s_vl_right.setAddress(EVE_TOF_ADDR_SECOND);
  s_vl_right.setDistanceMode(VL53L1X::Long);
  s_vl_right.setMeasurementTimingBudget(35000);
  s_vl_right.startContinuous(33);
  s_hw_dual = true;
  Serial.println(F("[EVE_TOF] Dual VL53L1X ready (XSHUT_SECOND)"));
#else
  if (s_vl_left.init()) {
    s_vl_left.setDistanceMode(VL53L1X::Long);
    s_vl_left.setMeasurementTimingBudget(35000);
    s_vl_left.startContinuous(33);
    s_hw_single = true;
    Serial.println(F("[EVE_TOF] Single VL53L1X (forward / center only)"));
  } else {
    Serial.println(F("[EVE_TOF] VL53L1X init failed"));
  }
#endif
#else
  Serial.println(F("[EVE_TOF] No VL53L1X.h — add Pololu library or EVE_TOF_SIMULATE=1"));
#endif
}

#if EVE_TOF_HAVE_VL53
static bool goodReading(VL53L1X& s, uint16_t mm) {
  if (s.timeoutOccurred()) {
    return false;
  }
  if (mm == 0 || mm >= EVE_TOF_FAR_IGNORE_MM) {
    return false;
  }
  return true;
}
#endif

void eveTofManagerPoll(uint32_t nowMs) {
#if !EVE_ENABLE_TOF
  (void)nowMs;
  return;
#endif

  if ((uint32_t)(nowMs - s_lastPollMs) < EVE_TOF_POLL_MS) {
    return;
  }
  s_lastPollMs = nowMs;

  EveTofRawFrame f;
  setInvalid(&f);

#if EVE_TOF_SIMULATE
  uint32_t t = (nowMs - s_simT0) / 3000u;
  switch (t % 5u) {
    case 0:
      f.left_mm = 600;
      f.valid_mask = 1u;
      break;
    case 1:
      f.right_mm = 650;
      f.valid_mask = 2u;
      break;
    case 2:
      f.center_mm = 700;
      f.valid_mask = 4u;
      break;
    case 3:
      f.left_mm = 550;
      f.right_mm = 580;
      f.valid_mask = 3u;
      break;
    default:
      f.valid_mask = 0u;
      break;
  }
  s_last = f;
  return;
#endif

#if EVE_TOF_HAVE_VL53
  if (s_hw_dual) {
    uint16_t dl = s_vl_left.read();
    if (goodReading(s_vl_left, dl) && (int32_t)dl <= EVE_TOF_FAR_IGNORE_MM) {
      f.left_mm = dl;
      f.valid_mask |= 1u;
    }
    uint16_t dr = s_vl_right.read();
    if (goodReading(s_vl_right, dr) && (int32_t)dr <= EVE_TOF_FAR_IGNORE_MM) {
      f.right_mm = dr;
      f.valid_mask |= 2u;
    }
  } else if (s_hw_single) {
    uint16_t dc = s_vl_left.read();
    if (goodReading(s_vl_left, dc) && (int32_t)dc <= EVE_TOF_FAR_IGNORE_MM) {
      f.center_mm = dc;
      f.valid_mask |= 4u;
    }
  }
#endif
  s_last = f;
}

bool eveTofManagerGetLastFrame(EveTofRawFrame* out) {
  if (!out) {
    return false;
  }
  *out = s_last;
  return s_last.valid_mask != 0;
}
