/**
 * EVE Phase O — immutable world facts (no intent, emotion, or behaviour).
 */
#pragma once

#include <stdint.h>

#include "eve_awareness_zones.h"

typedef struct {
  /* Person (O-2: ToF publisher fills these four fields only) */
  bool personPresent; /* derived: personConfidence >= EVE_AWARENESS_PERSON_PRESENT_THRESHOLD */
  float personDistanceMm;
  uint8_t personZone; /* EveAwarenessPersonZone */
  uint8_t personConfidence; /* 0–100 */

  /* Robot power / dock */
  bool batteryLow;
  float batteryVoltage;
  bool charging;
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
