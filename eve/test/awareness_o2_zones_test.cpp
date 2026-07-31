/**
 * Host-side acceptance check for O-2 zone mapping (no Arduino required).
 *
 *   g++ -std=c++11 -Wall -Wextra -o awareness_o2_zones_test awareness_o2_zones_test.cpp
 *   ./awareness_o2_zones_test
 */
#include "../awareness/eve_awareness_zones.h"

#include <cstdio>
#include <cstdlib>

static void expectZone(int32_t mm, uint8_t want, const char* label) {
  const uint8_t got = eveAwarenessZoneFromDistanceMm(mm, 2400);
  if (got != want) {
    std::fprintf(stderr, "FAIL %s: mm=%d want=%u got=%u (%s)\n", label, (int)mm,
                 (unsigned)want, (unsigned)got, eveAwarenessZoneName(got));
    std::exit(1);
  }
}

int main(void) {
  expectZone(-1, EVE_AWARENESS_ZONE_UNKNOWN, "invalid");
  expectZone(0, EVE_AWARENESS_ZONE_UNKNOWN, "zero");
  expectZone(2400, EVE_AWARENESS_ZONE_UNKNOWN, "far-ignore");

  /* Bench walk (acceptance table) */
  expectZone(2012, EVE_AWARENESS_ZONE_FAR, "1m-step FAR");
  expectZone(1018, EVE_AWARENESS_ZONE_MID, "500mm-step MID");
  expectZone(482, EVE_AWARENESS_ZONE_PERSONAL, "250mm-step PERSONAL");

  expectZone(800, EVE_AWARENESS_ZONE_NEAR, "NEAR band sample");

  std::printf("awareness_o2_zones_test: OK\n");
  return 0;
}
