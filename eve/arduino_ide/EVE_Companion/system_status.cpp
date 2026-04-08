#include "system_status.h"

static uint32_t s_bootMs = 0;

void systemStatusInit(void) {
  s_bootMs = millis();
}

void systemStatusTick(void) {
}

uint32_t systemStatusUptimeMs(void) {
  return millis() - s_bootMs;
}
