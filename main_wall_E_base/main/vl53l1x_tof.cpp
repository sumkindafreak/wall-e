/*******************************************************************************
 * vl53l1x_tof.cpp
 * VL53L1X forward-facing ToF for docking system
 ******************************************************************************/

#include "vl53l1x_tof.h"
#include <Wire.h>
#include <VL53L1X.h>
#include <Arduino.h>

static VL53L1X s_tof;
static bool s_initialized = false;
static uint16_t s_distance_mm = 0;
static bool s_valid = false;
static uint32_t s_last_read_ms = 0;
#define TOF_POLL_MS 50   /* ~20 Hz */

bool tofInit(void) {
  if (s_initialized) return s_valid;
  s_initialized = true;
  s_valid = false;
  s_distance_mm = 0;
  delay(1);
  yield();

  if (!s_tof.init()) {
    Serial.println("[ToF] VL53L1X not found (I2C 0x29)");
    return false;
  }
  s_tof.setDistanceMode(VL53L1X::Long);
  s_tof.setMeasurementTimingBudget(33000);
  s_tof.startContinuous(50);
  s_valid = true;
  Serial.println("[ToF] VL53L1X ready (forward-facing, dock system)");
  return true;
}

void tofUpdate(uint32_t now) {
  if (!s_initialized || !s_valid) return;
  if ((now - s_last_read_ms) < TOF_POLL_MS) return;
  s_last_read_ms = now;

  if (s_tof.dataReady()) {
    s_distance_mm = s_tof.read(false);
    if (s_distance_mm >= 8190) s_valid = false;
    else s_valid = true;
  }
}

uint16_t tofGetDistanceMm(void) {
  return s_distance_mm;
}

bool tofIsValid(void) {
  return s_valid;
}
