/*******************************************************************************
 * dock_sensors.cpp — Obstacle sensors only (debounced)
 ******************************************************************************/

#include "dock_sensors.h"
#include <Arduino.h>

bool gDockingEnabled = false;
static bool s_begun = false;

#define DEBOUNCE_MS  DOCK_DEBOUNCE_MS

static bool g_fl_raw, g_fr_raw, g_rl_raw, g_rr_raw;
static bool g_fl_stable, g_fr_stable, g_rl_stable, g_rr_stable;
static uint32_t g_fl_since, g_fr_since, g_rl_since, g_rr_since;
static bool g_fl_last, g_fr_last, g_rl_last, g_rr_last;

static uint32_t g_last_print_ms;
static bool g_last_front, g_last_rear, g_last_any;

static bool readWithInvert(int pin, bool invert) {
  bool v = (digitalRead(pin) == HIGH);
  return invert ? !v : v;
}

void dockSensorsBegin(void) {
  if (s_begun) return;
#ifndef DISABLE_DOCK_SENSORS
  pinMode(PIN_OBS_FRONT_L, INPUT_PULLUP);
  pinMode(PIN_OBS_FRONT_R, INPUT_PULLUP);
  pinMode(PIN_OBS_REAR_L, INPUT_PULLUP);
  pinMode(PIN_OBS_REAR_R, INPUT_PULLUP);

  g_fl_raw = readWithInvert(PIN_OBS_FRONT_L, INVERT_OBS_FRONT_L);
  g_fr_raw = readWithInvert(PIN_OBS_FRONT_R, INVERT_OBS_FRONT_R);
  g_rl_raw = readWithInvert(PIN_OBS_REAR_L, INVERT_OBS_REAR_L);
  g_rr_raw = readWithInvert(PIN_OBS_REAR_R, INVERT_OBS_REAR_R);
  g_fl_stable = g_fl_raw; g_fr_stable = g_fr_raw;
  g_rl_stable = g_rl_raw; g_rr_stable = g_rr_raw;
  g_fl_since = g_fr_since = g_rl_since = g_rr_since = millis();
  g_fl_last = g_fl_raw; g_fr_last = g_fr_raw; g_rl_last = g_rl_raw; g_rr_last = g_rr_raw;

  g_last_print_ms = 0;
  g_last_front = obstacleFrontBlocked();
  g_last_rear = obstacleRearBlocked();
  g_last_any = anyObstacleBlocked();
#endif
  s_begun = true;
}

static void applyDebounce(bool raw, bool& stable, uint32_t& since, bool& last_raw, uint32_t now) {
  if (raw != last_raw) {
    last_raw = raw;
    since = now;
  }
  if ((now - since) >= DEBOUNCE_MS) {
    stable = raw;
  }
}

void dockSensorsUpdate(void) {
  if (!s_begun) return;
#ifndef DISABLE_DOCK_SENSORS
  uint32_t now = millis();

  g_fl_raw = readWithInvert(PIN_OBS_FRONT_L, INVERT_OBS_FRONT_L);
  g_fr_raw = readWithInvert(PIN_OBS_FRONT_R, INVERT_OBS_FRONT_R);
  g_rl_raw = readWithInvert(PIN_OBS_REAR_L, INVERT_OBS_REAR_L);
  g_rr_raw = readWithInvert(PIN_OBS_REAR_R, INVERT_OBS_REAR_R);

  applyDebounce(g_fl_raw, g_fl_stable, g_fl_since, g_fl_last, now);
  applyDebounce(g_fr_raw, g_fr_stable, g_fr_since, g_fr_last, now);
  applyDebounce(g_rl_raw, g_rl_stable, g_rl_since, g_rl_last, now);
  applyDebounce(g_rr_raw, g_rr_stable, g_rr_since, g_rr_last, now);
#endif
}

bool dockBeamPresent(void) {
  return false;
}

bool obstacleFrontBlocked(void) {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_fl_stable || g_fr_stable;
#endif
}

bool dockObstacleFL(void)  {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_fl_stable;
#endif
}
bool dockObstacleFR(void)  {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_fr_stable;
#endif
}
bool dockObstacleRL(void)  {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_rl_stable;
#endif
}
bool dockObstacleRR(void)  {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_rr_stable;
#endif
}

bool obstacleRearBlocked(void) {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_rl_stable || g_rr_stable;
#endif
}

bool anyObstacleBlocked(void) {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_fl_stable || g_fr_stable || g_rl_stable || g_rr_stable;
#endif
}

void dockSensorsDebugPrint(uint32_t intervalMs) {
  uint32_t now = millis();
  bool front = obstacleFrontBlocked();
  bool rear = obstacleRearBlocked();
  bool any = anyObstacleBlocked();

  bool state_changed = (front != g_last_front) || (rear != g_last_rear) || (any != g_last_any);
  bool interval_elapsed = (now - g_last_print_ms) >= intervalMs;

  if (state_changed || interval_elapsed) {
    g_last_front = front;
    g_last_rear = rear;
    g_last_any = any;
    g_last_print_ms = now;

    Serial.printf("[DOCK] FL=%d FR=%d RL=%d RR=%d frontBlocked=%d rearBlocked=%d any=%d\n",
                  g_fl_stable ? 1 : 0, g_fr_stable ? 1 : 0,
                  g_rl_stable ? 1 : 0, g_rr_stable ? 1 : 0,
                  front ? 1 : 0, rear ? 1 : 0, any ? 1 : 0);
  }
}
