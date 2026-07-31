/**
 * EVE Phase O — immutable world facts (no intent, emotion, or behaviour).
 */
#pragma once

#include <stdint.h>

#include "eve_awareness_zones.h"
#include "eve_awareness_battery.h"

typedef struct {
  /* Person (O-2: ToF publisher fills these four fields only) */
  bool personPresent; /* derived: personConfidence >= EVE_AWARENESS_PERSON_PRESENT_THRESHOLD */
  float personDistanceMm;
  uint8_t personZone; /* EveAwarenessPersonZone */
  uint8_t personConfidence; /* 0–100 */

  /* Battery (O-3: battery_monitor — exactly these five facts) */
  bool batteryLow; /* derived: batteryPercent <= EVE_AWARENESS_BATTERY_LOW_PCT */
  float batteryVoltage;
  bool charging; /* derived: charge current > EVE_AWARENESS_CHARGING_MIN_A */
  int8_t batteryPercent; /* 0–100, or -1 if unknown */
  uint8_t batteryHealth; /* EveAwarenessBatteryHealth */

  /* Dock (separate publisher — not battery chemistry) */
  bool docked;

  /* Connectivity */
  bool wallELinked;
  bool remoteConnected;

  /* Audio */
  bool voiceDetected;
  bool audioPlaying;

  /* Timing */
  uint32_t uptimeMs;

  /* Subsystem health (facts, not policy) */
  bool sdMounted;
  bool displayReady;
  bool audioReady;
} EveAwarenessSnapshot;
