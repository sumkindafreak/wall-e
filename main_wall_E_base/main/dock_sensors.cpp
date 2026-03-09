/*******************************************************************************
 * dock_sensors.cpp
 * Docking and obstacle sensors — debounced, safe when unplugged
 ******************************************************************************/

#include "dock_sensors.h"
#include <Arduino.h>

bool gDockingEnabled = false;

/*=============================================================================
 * DEBOUNCE STATE
 *===========================================================================*/

#define DEBOUNCE_MS  DOCK_DEBOUNCE_MS

static bool g_beam_raw;
static bool g_beam_stable;
static uint32_t g_beam_stable_since;
static bool g_beam_last_raw;

static bool g_fl_raw, g_fr_raw, g_rl_raw, g_rr_raw;
static bool g_fl_stable, g_fr_stable, g_rl_stable, g_rr_stable;
static uint32_t g_fl_since, g_fr_since, g_rl_since, g_rr_since;
static bool g_fl_last, g_fr_last, g_rl_last, g_rr_last;

static uint32_t g_last_print_ms;
static bool g_last_beam, g_last_front, g_last_rear, g_last_any;

static bool readWithInvert(int pin, bool invert) {
  bool v = (digitalRead(pin) == HIGH);
  return invert ? !v : v;
}

void dockSensorsBegin(void) {
  pinMode(PIN_DOCK_IR_BEAM, INPUT_PULLUP);
  pinMode(PIN_OBS_FRONT_L, INPUT_PULLUP);
  pinMode(PIN_OBS_FRONT_R, INPUT_PULLUP);
  pinMode(PIN_OBS_REAR_L, INPUT_PULLUP);
  pinMode(PIN_OBS_REAR_R, INPUT_PULLUP);

  g_beam_raw = readWithInvert(PIN_DOCK_IR_BEAM, INVERT_DOCK_IR_BEAM);
  g_beam_stable = g_beam_raw;
  g_beam_stable_since = millis();
  g_beam_last_raw = g_beam_raw;

  g_fl_raw = readWithInvert(PIN_OBS_FRONT_L, INVERT_OBS_FRONT_L);
  g_fr_raw = readWithInvert(PIN_OBS_FRONT_R, INVERT_OBS_FRONT_R);
  g_rl_raw = readWithInvert(PIN_OBS_REAR_L, INVERT_OBS_REAR_L);
  g_rr_raw = readWithInvert(PIN_OBS_REAR_R, INVERT_OBS_REAR_R);
  g_fl_stable = g_fl_raw; g_fr_stable = g_fr_raw;
  g_rl_stable = g_rl_raw; g_rr_stable = g_rr_raw;
  g_fl_since = g_fr_since = g_rl_since = g_rr_since = millis();
  g_fl_last = g_fl_raw; g_fr_last = g_fr_raw; g_rl_last = g_rl_raw; g_rr_last = g_rr_raw;

  g_last_print_ms = 0;
  g_last_beam = g_beam_stable;
  g_last_front = obstacleFrontBlocked();
  g_last_rear = obstacleRearBlocked();
  g_last_any = anyObstacleBlocked();
}

static void applyDebounce(bool raw, bool& stable, uint32_t& since, bool& last_raw, uint32_t now) {
  if (raw != last_raw) {
    last_raw = raw;
    since = now;  /* raw changed, reset timer */
  }
  if ((now - since) >= DEBOUNCE_MS) {
    stable = raw;
  }
}

void dockSensorsUpdate(void) {
  uint32_t now = millis();

  g_beam_raw = readWithInvert(PIN_DOCK_IR_BEAM, INVERT_DOCK_IR_BEAM);
  applyDebounce(g_beam_raw, g_beam_stable, g_beam_stable_since, g_beam_last_raw, now);

  g_fl_raw = readWithInvert(PIN_OBS_FRONT_L, INVERT_OBS_FRONT_L);
  g_fr_raw = readWithInvert(PIN_OBS_FRONT_R, INVERT_OBS_FRONT_R);
  g_rl_raw = readWithInvert(PIN_OBS_REAR_L, INVERT_OBS_REAR_L);
  g_rr_raw = readWithInvert(PIN_OBS_REAR_R, INVERT_OBS_REAR_R);

  applyDebounce(g_fl_raw, g_fl_stable, g_fl_since, g_fl_last, now);
  applyDebounce(g_fr_raw, g_fr_stable, g_fr_since, g_fr_last, now);
  applyDebounce(g_rl_raw, g_rl_stable, g_rl_since, g_rl_last, now);
  applyDebounce(g_rr_raw, g_rr_stable, g_rr_since, g_rr_last, now);
}

bool dockBeamPresent(void) {
  return g_beam_stable;
}

bool obstacleFrontBlocked(void) {
  return g_fl_stable || g_fr_stable;
}

bool dockObstacleFL(void)  { return g_fl_stable; }
bool dockObstacleFR(void)  { return g_fr_stable; }
bool dockObstacleRL(void)  { return g_rl_stable; }
bool dockObstacleRR(void)  { return g_rr_stable; }

bool obstacleRearBlocked(void) {
  return g_rl_stable || g_rr_stable;
}

bool anyObstacleBlocked(void) {
  return g_fl_stable || g_fr_stable || g_rl_stable || g_rr_stable;
}

void dockSensorsDebugPrint(uint32_t intervalMs) {
  uint32_t now = millis();
  bool beam = dockBeamPresent();
  bool front = obstacleFrontBlocked();
  bool rear = obstacleRearBlocked();
  bool any = anyObstacleBlocked();

  bool state_changed = (beam != g_last_beam) || (front != g_last_front) ||
                       (rear != g_last_rear) || (any != g_last_any);
  bool interval_elapsed = (now - g_last_print_ms) >= intervalMs;

  if (state_changed || interval_elapsed) {
    g_last_beam = beam;
    g_last_front = front;
    g_last_rear = rear;
    g_last_any = any;
    g_last_print_ms = now;

    Serial.printf("[DOCK] beam=%d FL=%d FR=%d RL=%d RR=%d frontBlocked=%d rearBlocked=%d any=%d\n",
                  beam ? 1 : 0,
                  g_fl_stable ? 1 : 0, g_fr_stable ? 1 : 0,
                  g_rl_stable ? 1 : 0, g_rr_stable ? 1 : 0,
                  front ? 1 : 0, rear ? 1 : 0, any ? 1 : 0);
  }
}
