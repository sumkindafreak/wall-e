/*******************************************************************************
 * dock_sensors.cpp
 * ACS712 current sensor, IR beam, and obstacle sensors
 ******************************************************************************/

#include "dock_config.h"
#include "dock_hw.h"
#include "dock_sensors.h"
#include "dock_sonar.h"
#include "dock_vl6180.h"
#include <Arduino.h>

/*=============================================================================
 * STATE
 *===========================================================================*/

static int   g_acs_zero = 2048;      /* Calibrated zero point (12-bit midpoint) */
static int   g_acs_raw = 2048;
static float g_acs_amps = 0.0f;
static int   g_acs_buffer[ACS712_MOVING_AVG_SAMPLES];
static int   g_acs_index = 0;
static bool  g_acs_filled = false;
static bool  g_beam_present = false;
static bool  g_mouth_blocked = false;
static bool  g_obstacle[4] = {false, false, false, false};  /* 0=FL, 1=FR, 2=BL, 3=BR */
static bool  g_acs_available = false;

#define ADC_MAX_12BIT  4095
#define VREF_MV        3300

/*=============================================================================
 * IMPLEMENTATION
 *===========================================================================*/

void dockSensorsBegin(void) {
#if !S3_CURRENT_ONLY_MODE
  // Obstacle digital inputs only (beam hardware removed on this build)
#if USE_OBSTACLE_SENSORS
  /* Pull-up. OBSTACLE_ACTIVE_HIGH 1 = blocked when pin HIGH; 0 = blocked when pin LOW */
  dockConfigureInputPin(PIN_OBSTACLE_1, INPUT_PULLUP, "obstacle 1");
  dockConfigureInputPin(PIN_OBSTACLE_2, INPUT_PULLUP, "obstacle 2");
  dockConfigureInputPin(PIN_OBSTACLE_3, INPUT_PULLUP, "obstacle 3");
  dockConfigureInputPin(PIN_OBSTACLE_4, INPUT_PULLUP, "obstacle 4");
#endif
#else
  // S3_CURRENT_ONLY_MODE: force safe defaults for sensors
  g_beam_present = false;
  g_mouth_blocked = false;
#endif

  // Current sense: only enable if the ACS712 pin is valid & ADC-capable
  g_acs_available = dockCurrentSenseAvailable();
  if (!g_acs_available) {
    g_acs_zero = 0;
    g_acs_raw = 0;
    g_acs_amps = 0.0f;
    for (int i = 0; i < ACS712_MOVING_AVG_SAMPLES; i++) {
      g_acs_buffer[i] = 0;
    }
    g_acs_filled = true;
    g_acs_index = 0;
    return;
  }

  dockConfigureInputPin(PIN_ACS712_ADC, INPUT, "ACS712 current sense");

  // Fill moving-average buffer with initial readings
  for (int i = 0; i < ACS712_MOVING_AVG_SAMPLES; i++) {
    g_acs_buffer[i] = dockAnalogReadSafe(PIN_ACS712_ADC, "ACS712 current sense");
  }
  g_acs_filled = true;
  g_acs_index = 0;

  // TEMP: skip long blocking calibration on ESP32-S3 to avoid watchdog resets.
  // Use the initial average as zero and let the moving average settle in loop().
  long sum = 0;
  for (int i = 0; i < ACS712_MOVING_AVG_SAMPLES; i++) {
    sum += g_acs_buffer[i];
  }
  g_acs_zero = sum / ACS712_MOVING_AVG_SAMPLES;
}

void dockSensorsUpdate(void) {
#if USE_VL6180_TOF
  dockVl6180Update();
#endif
#if USE_SONAR
  dockSonarUpdate();
#endif

  // IR + obstacle sensors
#if !S3_CURRENT_ONLY_MODE
#if USE_OBSTACLE_SENSORS
  int obstacle_inactive = OBSTACLE_ACTIVE_HIGH ? LOW : HIGH;
  g_obstacle[0] = (dockDigitalReadSafe(PIN_OBSTACLE_1, "obstacle 1", obstacle_inactive) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  g_obstacle[1] = (dockDigitalReadSafe(PIN_OBSTACLE_2, "obstacle 2", obstacle_inactive) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  g_obstacle[2] = (dockDigitalReadSafe(PIN_OBSTACLE_3, "obstacle 3", obstacle_inactive) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  g_obstacle[3] = (dockDigitalReadSafe(PIN_OBSTACLE_4, "obstacle 4", obstacle_inactive) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  g_mouth_blocked = g_obstacle[0] || g_obstacle[1] || g_obstacle[2] || g_obstacle[3];
#else
  g_mouth_blocked = false;
  g_obstacle[0] = g_obstacle[1] = g_obstacle[2] = g_obstacle[3] = false;
#endif
#else
  g_beam_present = false;
  g_mouth_blocked = false;
  g_obstacle[0] = g_obstacle[1] = g_obstacle[2] = g_obstacle[3] = false;
#endif

  // ACS712: read and update moving average
  if (!g_acs_available) {
    g_acs_raw = 0;
    g_acs_amps = 0.0f;
    return;
  }

  g_acs_raw = dockAnalogReadSafe(PIN_ACS712_ADC, "ACS712 current sense");
  g_acs_buffer[g_acs_index] = g_acs_raw;
  g_acs_index = (g_acs_index + 1) % ACS712_MOVING_AVG_SAMPLES;

  long avg = 0;
  for (int i = 0; i < ACS712_MOVING_AVG_SAMPLES; i++) {
    avg += g_acs_buffer[i];
  }
  int avg_raw = avg / ACS712_MOVING_AVG_SAMPLES;

  // Convert ADC reading to millivolts, then to amps
  int v_mv = (avg_raw - g_acs_zero) * VREF_MV / ADC_MAX_12BIT;
  g_acs_amps = (float)v_mv / (float)ACS712_MV_PER_AMP;
  g_acs_amps = g_acs_amps * CURRENT_CALIB_SCALE + CURRENT_CALIB_OFFSET;
}

bool dockBeamPresent(void) {
  /* Beam hardware removed: treat "beam present" as generic dock-detected for legacy callers. */
  return dockDockDetected();
}

bool dockDockDetected(void) {
#if USE_VL6180_TOF
  if (dockVl6180Ready()) {
    return dockVl6180Docked();
  }
#endif
#if USE_SONAR
  return dockSonarInRange();
#else
  return g_mouth_blocked;
#endif
}

bool dockMouthBlocked(void) {
  return g_mouth_blocked;
}

bool dockAtLeastNObstaclesBlocked(int n) {
  int count = 0;
  for (int i = 0; i < 4; i++)
    if (g_obstacle[i]) count++;
  return (count >= n);
}

bool dockObstaclesAllClear(void) {
  return !g_obstacle[0] && !g_obstacle[1] && !g_obstacle[2] && !g_obstacle[3];
}

bool dockObstacleBlocked(int index) {
  if (index < 0 || index > 3) return false;
  return g_obstacle[index];
}

int dockCurrentRaw(void) {
  return g_acs_raw;
}

int dockCurrentZero(void) {
  return g_acs_zero;
}

float dockCurrentAmps(void) {
  return g_acs_amps;
}

uint32_t dockCurrentVoltageMv(void) {
  return (uint32_t)g_acs_raw * VREF_MV / ADC_MAX_12BIT;
}
