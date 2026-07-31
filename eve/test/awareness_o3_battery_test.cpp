/**
 * Host-side check for O-3 derived battery facts (no Arduino required).
 */
#include "../awareness/eve_awareness_battery.h"

#include <cstdio>
#include <cstdlib>

int main(void) {
  if (eveAwarenessBatteryHealthFromStatus(EVE_AWARENESS_BAT_STATUS_OK, true) !=
      EVE_AWARENESS_BATTERY_HEALTH_OK) {
    std::fprintf(stderr, "FAIL health OK\n");
    return 1;
  }
  if (eveAwarenessBatteryHealthFromStatus(EVE_AWARENESS_BAT_STATUS_WARN, true) !=
      EVE_AWARENESS_BATTERY_HEALTH_WARN) {
    std::fprintf(stderr, "FAIL health WARN\n");
    return 1;
  }
  if (!eveAwarenessChargingFromCurrentA(0.12f, true, 0.04f)) {
    std::fprintf(stderr, "FAIL charging true\n");
    return 1;
  }
  if (eveAwarenessChargingFromCurrentA(0.01f, true, 0.04f)) {
    std::fprintf(stderr, "FAIL charging false\n");
    return 1;
  }
  if (!eveAwarenessBatteryLowFromPercent(20, 25)) {
    std::fprintf(stderr, "FAIL batteryLow at 20%%\n");
    return 1;
  }
  if (eveAwarenessBatteryLowFromPercent(80, 25)) {
    std::fprintf(stderr, "FAIL batteryLow false at 80%%\n");
    return 1;
  }

  std::printf("awareness_o3_battery_test: OK\n");
  return 0;
}
