/*******************************************************************************
 * dock_sensors.cpp
 * ACS712 current sensor, IR beam, and obstacle sensors
 ******************************************************************************/

#include "dock_config.h"
#include "dock_sensors.h"
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

#define ADC_MAX_12BIT  4095
#define VREF_MV        3300

/*=============================================================================
 * IMPLEMENTATION
 *===========================================================================*/

void dockSensorsBegin(void) {
  pinMode(PIN_IR_BEAM, INPUT_PULLUP);
#if USE_OBSTACLE_SENSORS
  pinMode(PIN_OBSTACLE_1, INPUT);
  pinMode(PIN_OBSTACLE_2, INPUT);
  pinMode(PIN_OBSTACLE_3, INPUT);
  pinMode(PIN_OBSTACLE_4, INPUT);
#endif
  pinMode(PIN_ACS712_ADC, INPUT);

  /* Fill buffer with initial readings */
  for (int i = 0; i < ACS712_MOVING_AVG_SAMPLES; i++) {
    g_acs_buffer[i] = analogRead(PIN_ACS712_ADC);
  }
  g_acs_filled = true;
  g_acs_index = 0;

  /* Calibration: average N samples with charging OFF */
  long sum = 0;
  for (int i = 0; i < ACS712_CALIB_SAMPLES; i++) {
    sum += analogRead(PIN_ACS712_ADC);
    delay(2);
  }
  g_acs_zero = sum / ACS712_CALIB_SAMPLES;
}

void dockSensorsUpdate(void) {
  /* Read IR beam: LOW = beam broken = present (typical break-beam) */
  /* If your sensor is inverted, change to !digitalRead(...) */
  g_beam_present = (digitalRead(PIN_IR_BEAM) == LOW);

#if USE_OBSTACLE_SENSORS
  bool o1 = (digitalRead(PIN_OBSTACLE_1) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  bool o2 = (digitalRead(PIN_OBSTACLE_2) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  bool o3 = (digitalRead(PIN_OBSTACLE_3) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  bool o4 = (digitalRead(PIN_OBSTACLE_4) == (OBSTACLE_ACTIVE_HIGH ? HIGH : LOW));
  g_mouth_blocked = o1 || o2 || o3 || o4;
#else
  g_mouth_blocked = false;
#endif

  /* ACS712: read and update moving average */
  g_acs_raw = analogRead(PIN_ACS712_ADC);
  g_acs_buffer[g_acs_index] = g_acs_raw;
  g_acs_index = (g_acs_index + 1) % ACS712_MOVING_AVG_SAMPLES;

  long avg = 0;
  for (int i = 0; i < ACS712_MOVING_AVG_SAMPLES; i++) {
    avg += g_acs_buffer[i];
  }
  int avg_raw = avg / ACS712_MOVING_AVG_SAMPLES;

  /* Convert to voltage then current */
  /* V_mv = (adc / 4095) * 3300 */
  int v_mv = (avg_raw - g_acs_zero) * VREF_MV / ADC_MAX_12BIT;
  /* I_A = V_mv / (mV_per_A) */
  g_acs_amps = (float)v_mv / (float)ACS712_MV_PER_AMP;
}

bool dockBeamPresent(void) {
  return g_beam_present;
}

bool dockMouthBlocked(void) {
  return g_mouth_blocked;
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
