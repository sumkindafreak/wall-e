#include "eve_status_manager.h"

#include "eve_attachment_manager.h"
#include "state_machine.h"

void eveStatusManagerInit(void) {
  Serial.println(F("[BOOT][EVE] status manager"));
}

void eveStatusManagerTick(void) {}

/**
 * Status for the docked web page and the serial console: the control core's full status
 * (mode, stop, faults, link, battery, outputs) plus the legacy keys the page already reads.
 */
String eveStatusManagerGetJSON(void) {
  char core[900];
  const size_t n = stateMachineStatusJson(core, sizeof(core));
  String j;
  j.reserve(n + 128);
  if (n > 1 && n < sizeof(core) && core[n - 1] == '}') {
    core[n - 1] = '\0';
    j = core;
    j += ",";
  } else {
    j = "{";
  }
  j += "\"ok\":true";
  j += ",\"state\":\"";
  j += stateMachineGetStateName();
  j += "\",\"uart_peer\":\"";
  j += stateMachineGetPeerLabel();
  j += "\",\"attached\":";
  j += eveAttachmentIsAttached() ? "true" : "false";
  j += "}";
  return j;
}
