// ============================================================
//  System status — ESP-NOW telemetry age + rates
// ============================================================

#include "system_status.h"
#include "espnow_control.h"
#include "packet_control.h"
#include "sd_manager.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

static uint32_t s_lastOkMs = 0;

void systemStatusInit(void) {
  s_lastOkMs = millis();
}

void systemStatusTick(uint32_t now_ms) {
  if (packetTelemetryValid()) {
    s_lastOkMs = now_ms;
  }
}

bool systemStatusEspNowLinkOk(void) {
  return espnowGetPacketRate() > 0 || packetTelemetryValid();
}

bool systemStatusTelemetryFresh(uint32_t maxAgeMs) {
  return (millis() - s_lastOkMs) <= maxAgeMs;
}

void systemStatusFormatDebug(char* buf, size_t bufLen) {
  if (!buf || bufLen < 8) return;
  uint32_t age = millis() - s_lastOkMs;
  snprintf(buf, bufLen, "ESPNOW %up/s TELEM_AGE %lums SD_%s",
           (unsigned)espnowGetPacketRate(), (unsigned long)age,
           sdIsAvailable() ? "OK" : "--");
}
