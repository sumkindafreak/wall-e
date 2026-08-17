/*******************************************************************************
 * dock_sensors.cpp — four debounced obstacle sensors
 *
 * ESP32-P4: PCF8574 @ 0x20 on the shared 3.3 V I2C bus.
 * Legacy S3: direct GPIO inputs.
 ******************************************************************************/

#include "dock_sensors.h"
#include <Arduino.h>
#include <Wire.h>

bool gDockingEnabled = false;
static bool s_begun = false;

#define DEBOUNCE_MS DOCK_DEBOUNCE_MS

static bool g_fl_raw, g_fr_raw, g_rl_raw, g_rr_raw;
static bool g_fl_stable, g_fr_stable, g_rl_stable, g_rr_stable;
static uint32_t g_fl_since, g_fr_since, g_rl_since, g_rr_since;
static bool g_fl_last, g_fr_last, g_rl_last, g_rr_last;

static uint32_t g_last_print_ms;
static bool g_last_front, g_last_rear, g_last_any;

#if WALLE_BASE_OBSTACLES_PCF8574
static bool s_pcfReady = false;
static uint8_t s_pcfLastPort = 0xFF;
static uint32_t s_pcfLastGoodMs = 0;
static uint32_t s_pcfLastWarnMs = 0;

static bool pcf8574Write(uint8_t value) {
  Wire.beginTransmission(BASE_OBS_PCF8574_ADDR);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool pcf8574Read(uint8_t* out) {
  if (!out) return false;
  const uint8_t count = Wire.requestFrom((uint8_t)BASE_OBS_PCF8574_ADDR,
                                         (uint8_t)1);
  if (count != 1 || Wire.available() < 1) return false;
  *out = (uint8_t)Wire.read();
  return true;
}

static bool pcf8574Begin() {
  // PCF8574 quasi-bidirectional pins become inputs by writing HIGH.
  if (!pcf8574Write(0xFF)) return false;
  delay(1);
  uint8_t port = 0xFF;
  if (!pcf8574Read(&port)) return false;
  s_pcfLastPort = port;
  s_pcfLastGoodMs = millis();
  return true;
}

static bool pcfReadBit(uint8_t bit, bool invert) {
  const bool v = ((s_pcfLastPort >> bit) & 0x01u) != 0;
  return invert ? !v : v;
}

static bool updatePcfPort(uint32_t now) {
  uint8_t port = 0xFF;
  if (pcf8574Read(&port)) {
    s_pcfLastPort = port;
    s_pcfLastGoodMs = now;
    return true;
  }

  if (now - s_pcfLastWarnMs >= 2000u) {
    s_pcfLastWarnMs = now;
    Serial.println(F("[DOCK] WARN: PCF8574 obstacle expander read failed"));
  }
  return false;
}
#else
static bool readWithInvert(int pin, bool invert) {
  const bool v = (digitalRead(pin) == HIGH);
  return invert ? !v : v;
}
#endif

static void sampleRawSensors(bool failSafeBlocked) {
#if WALLE_BASE_OBSTACLES_PCF8574
  if (failSafeBlocked) {
    // Losing a configured obstacle bus while running must never look "clear".
    g_fl_raw = g_fr_raw = g_rl_raw = g_rr_raw = true;
    return;
  }
  g_fl_raw = pcfReadBit(BASE_OBS_PCF8574_FRONT_L_BIT, INVERT_OBS_FRONT_L);
  g_fr_raw = pcfReadBit(BASE_OBS_PCF8574_FRONT_R_BIT, INVERT_OBS_FRONT_R);
  g_rl_raw = pcfReadBit(BASE_OBS_PCF8574_REAR_L_BIT, INVERT_OBS_REAR_L);
  g_rr_raw = pcfReadBit(BASE_OBS_PCF8574_REAR_R_BIT, INVERT_OBS_REAR_R);
#else
  (void)failSafeBlocked;
  g_fl_raw = readWithInvert(PIN_OBS_FRONT_L, INVERT_OBS_FRONT_L);
  g_fr_raw = readWithInvert(PIN_OBS_FRONT_R, INVERT_OBS_FRONT_R);
  g_rl_raw = readWithInvert(PIN_OBS_REAR_L, INVERT_OBS_REAR_L);
  g_rr_raw = readWithInvert(PIN_OBS_REAR_R, INVERT_OBS_REAR_R);
#endif
}

void dockSensorsBegin(void) {
  if (s_begun) return;
#ifndef DISABLE_DOCK_SENSORS
#if WALLE_BASE_OBSTACLES_PCF8574
  s_pcfReady = pcf8574Begin();
  if (s_pcfReady) {
    Serial.printf("[DOCK] Obstacle inputs: PCF8574 ready at 0x%02X (P0..P3)\n",
                  BASE_OBS_PCF8574_ADDR);
    sampleRawSensors(false);
  } else {
    Serial.printf("[DOCK] WARN: PCF8574 not found at 0x%02X; obstacles fail-safe BLOCKED\n",
                  BASE_OBS_PCF8574_ADDR);
    sampleRawSensors(true);
  }
#else
  pinMode(PIN_OBS_FRONT_L, INPUT_PULLUP);
  pinMode(PIN_OBS_FRONT_R, INPUT_PULLUP);
  pinMode(PIN_OBS_REAR_L, INPUT_PULLUP);
  pinMode(PIN_OBS_REAR_R, INPUT_PULLUP);
  sampleRawSensors(false);
#endif

  g_fl_stable = g_fl_raw;
  g_fr_stable = g_fr_raw;
  g_rl_stable = g_rl_raw;
  g_rr_stable = g_rr_raw;
  g_fl_since = g_fr_since = g_rl_since = g_rr_since = millis();
  g_fl_last = g_fl_raw;
  g_fr_last = g_fr_raw;
  g_rl_last = g_rl_raw;
  g_rr_last = g_rr_raw;

  g_last_print_ms = 0;
  g_last_front = obstacleFrontBlocked();
  g_last_rear = obstacleRearBlocked();
  g_last_any = anyObstacleBlocked();
#endif
  s_begun = true;
}

static void applyDebounce(bool raw,
                          bool& stable,
                          uint32_t& since,
                          bool& last_raw,
                          uint32_t now) {
  if (raw != last_raw) {
    last_raw = raw;
    since = now;
  }
  if ((now - since) >= DEBOUNCE_MS) stable = raw;
}

void dockSensorsUpdate(void) {
  if (!s_begun) return;
#ifndef DISABLE_DOCK_SENSORS
  const uint32_t now = millis();

#if WALLE_BASE_OBSTACLES_PCF8574
  bool readOk = false;
  if (s_pcfReady) readOk = updatePcfPort(now);
  else {
    // Retry a missing expander periodically so hot-plug/late power-up recovers.
    static uint32_t lastRetryMs = 0;
    if (now - lastRetryMs >= 2000u) {
      lastRetryMs = now;
      s_pcfReady = pcf8574Begin();
      if (s_pcfReady) Serial.println(F("[DOCK] PCF8574 obstacle expander recovered"));
    }
    readOk = s_pcfReady;
  }
  sampleRawSensors(!readOk);
#else
  sampleRawSensors(false);
#endif

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

bool dockObstacleFL(void) {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_fl_stable;
#endif
}

bool dockObstacleFR(void) {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_fr_stable;
#endif
}

bool dockObstacleRL(void) {
#ifdef DISABLE_DOCK_SENSORS
  return false;
#else
  return g_rl_stable;
#endif
}

bool dockObstacleRR(void) {
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
  const uint32_t now = millis();
  const bool front = obstacleFrontBlocked();
  const bool rear = obstacleRearBlocked();
  const bool any = anyObstacleBlocked();

  const bool stateChanged =
      (front != g_last_front) ||
      (rear != g_last_rear) ||
      (any != g_last_any);
  const bool intervalElapsed = (now - g_last_print_ms) >= intervalMs;

  if (stateChanged || intervalElapsed) {
    g_last_front = front;
    g_last_rear = rear;
    g_last_any = any;
    g_last_print_ms = now;

    Serial.printf("[DOCK] FL=%d FR=%d RL=%d RR=%d frontBlocked=%d rearBlocked=%d any=%d",
                  g_fl_stable ? 1 : 0,
                  g_fr_stable ? 1 : 0,
                  g_rl_stable ? 1 : 0,
                  g_rr_stable ? 1 : 0,
                  front ? 1 : 0,
                  rear ? 1 : 0,
                  any ? 1 : 0);
#if WALLE_BASE_OBSTACLES_PCF8574
    Serial.printf(" pcf=%s age=%lums",
                  s_pcfReady ? "OK" : "FAIL",
                  (unsigned long)(now - s_pcfLastGoodMs));
#endif
    Serial.println();
  }
}
