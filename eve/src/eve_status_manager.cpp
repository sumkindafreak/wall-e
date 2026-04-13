#include "eve_status_manager.h"
#include "system_status.h"
#include "eve_attachment_manager.h"
#include "battery_monitor.h"

void eveStatusManagerInit(void) {
  Serial.println(F("[BOOT][EVE] status manager"));
}

void eveStatusManagerTick(void) {}

String eveStatusManagerGetJSON(void) {
  String j = "{\"ok\":true";
  j += ",\"uptime_ms\":"; j += String(systemStatusUptimeMs());
  j += ",\"heap\":"; j += String(ESP.getFreeHeap());
  j += ",\"attached\":"; j += eveAttachmentIsAttached() ? "true" : "false";
  j += ",\"bat_hw\":"; j += eveBatteryHardwareEnabled() ? "true" : "false";
  j += ",\"bat_ok\":"; j += eveBatteryDataValid() ? "true" : "false";
  j += ",\"bat_v\":"; j += String(eveBatteryVoltage(), 2);
  j += ",\"bat_a\":"; j += String(eveBatteryCurrentA(), 2);
  j += ",\"bat_pct\":"; j += String(eveBatteryPercent());
  j += "}";
  return j;
}
