/**
 * diagnostics.cpp — Print diagnostic block to Serial
 */
#include "diagnostics.h"
#include <stdio.h>

static unsigned long s_lastDiag = 0;

void diagnosticsInit() {
  s_lastDiag = 0;
}

void diagnosticsTick() {
#if !DIAGNOSTICS_ENABLE
  return;
#endif
  unsigned long now = millis();
  if (now - s_lastDiag < DIAGNOSTICS_INTERVAL_MS) return;
  s_lastDiag = now;

  Serial.println(F("--- DIAG ---"));
  Serial.printf(F("Mode: %s\n"), modeToString(getSystemMode()));
  Serial.printf(F("Mic L:%d R:%d Dir:%s\n"), micGetLeftLevel(), micGetRightLevel(), micDirToString(micGetDirection()));
  Serial.printf(F("Dock IR: %s\n"), dockIrToString(irDockGetState()));
  Serial.printf(F("Audio busy: %d\n"), audioIsBusy() ? 1 : 0);
  Serial.printf(F("Heap: %u\n"), (unsigned)ESP.getFreeHeap());
  Serial.println(F("------------"));
}
