/**
 * EVE Phase O — immutable world facts (no intent, emotion, or behaviour).
 */
#pragma once

#include <stdint.h>

typedef struct {
  /* Person (O-2: ToF publisher fills these) */
  bool personPresent;
  float personDistanceMm;
  uint8_t personZone;
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
