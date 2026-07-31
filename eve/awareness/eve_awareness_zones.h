/**
 * EVE Phase O-2 — person distance zones (stable API; retune thresholds in config only).
 */
#pragma once

#include <stdint.h>

typedef enum {
  EVE_AWARENESS_ZONE_UNKNOWN = 0,
  EVE_AWARENESS_ZONE_FAR,
  EVE_AWARENESS_ZONE_MID,
  EVE_AWARENESS_ZONE_NEAR,
  EVE_AWARENESS_ZONE_PERSONAL,
} EveAwarenessPersonZone;

#ifndef EVE_AWARENESS_ZONE_PERSONAL_MAX_MM
#define EVE_AWARENESS_ZONE_PERSONAL_MAX_MM 550
#endif
#ifndef EVE_AWARENESS_ZONE_NEAR_MAX_MM
#define EVE_AWARENESS_ZONE_NEAR_MAX_MM 900
#endif
#ifndef EVE_AWARENESS_ZONE_MID_MAX_MM
#define EVE_AWARENESS_ZONE_MID_MAX_MM 1500
#endif

static inline uint8_t eveAwarenessZoneFromDistanceMm(int32_t distanceMm, int32_t farIgnoreMm) {
  if (distanceMm <= 0 || distanceMm >= farIgnoreMm) {
    return EVE_AWARENESS_ZONE_UNKNOWN;
  }
  if (distanceMm <= EVE_AWARENESS_ZONE_PERSONAL_MAX_MM) {
    return EVE_AWARENESS_ZONE_PERSONAL;
  }
  if (distanceMm <= EVE_AWARENESS_ZONE_NEAR_MAX_MM) {
    return EVE_AWARENESS_ZONE_NEAR;
  }
  if (distanceMm <= EVE_AWARENESS_ZONE_MID_MAX_MM) {
    return EVE_AWARENESS_ZONE_MID;
  }
  return EVE_AWARENESS_ZONE_FAR;
}

static inline const char* eveAwarenessZoneName(uint8_t zone) {
  switch (zone) {
    case EVE_AWARENESS_ZONE_FAR:
      return "FAR";
    case EVE_AWARENESS_ZONE_MID:
      return "MID";
    case EVE_AWARENESS_ZONE_NEAR:
      return "NEAR";
    case EVE_AWARENESS_ZONE_PERSONAL:
      return "PERSONAL";
    default:
      return "UNKNOWN";
  }
}
