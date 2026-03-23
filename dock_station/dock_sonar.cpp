/*******************************************************************************
 * dock_sonar.cpp
 * HC-SR04 sonar: last-resort dock detection when IR beam doesn't register.
 * Future expansion: use distance for approach staging / arrow guidance (e.g. <50 cm = precision).
 ******************************************************************************/

#include "dock_sonar.h"
#include "dock_config.h"
#include <Arduino.h>

#if USE_SONAR

static float g_distance_cm = 0.0f;
static uint32_t g_last_ping_ms = 0;
#define SONAR_PING_INTERVAL_MS  60   /* ~16 Hz max for HC-SR04 */

static void ping(void) {
  pinMode(PIN_SONAR_TRIG, OUTPUT);
  digitalWrite(PIN_SONAR_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_SONAR_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_SONAR_TRIG, LOW);

  pinMode(PIN_SONAR_ECHO, INPUT);
  unsigned long us = pulseIn(PIN_SONAR_ECHO, HIGH, 30000);  /* 30ms timeout ~5m */
  if (us == 0) {
    g_distance_cm = 0.0f;  /* no echo */
    return;
  }
  /* us to cm: speed of sound ~343 m/s => 29.4 us/cm; common formula us/58 ≈ cm */
  float cm = (float)us / 58.0f;
  if (cm > (float)SONAR_MAX_CM) cm = 0.0f;
  g_distance_cm = cm;
}

void dockSonarBegin(void) {
  pinMode(PIN_SONAR_TRIG, OUTPUT);
  digitalWrite(PIN_SONAR_TRIG, LOW);
  pinMode(PIN_SONAR_ECHO, INPUT);
  g_distance_cm = 0.0f;
  g_last_ping_ms = 0;
}

void dockSonarUpdate(void) {
  uint32_t now = millis();
  if (now - g_last_ping_ms >= SONAR_PING_INTERVAL_MS) {
    g_last_ping_ms = now;
    ping();
  }
}

float dockSonarDistanceCm(void) {
  return g_distance_cm;
}

bool dockSonarInRange(void) {
  return (g_distance_cm > 0.0f && g_distance_cm < (float)SONAR_DOCKED_CM);
}

#else  /* !USE_SONAR */

void dockSonarBegin(void) {}
void dockSonarUpdate(void) {}
float dockSonarDistanceCm(void) { return 0.0f; }
bool dockSonarInRange(void) { return false; }

#endif /* USE_SONAR */
