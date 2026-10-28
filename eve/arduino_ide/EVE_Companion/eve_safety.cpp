#include "eve_safety.h"

#include <stdio.h>

static EveSafetyHooks s_hooks = {nullptr, nullptr};
static bool s_stopped = false;
static EveStopSource s_src = EVE_STOP_SRC_NONE;
static uint32_t s_stopCount = 0;
static uint16_t s_faults = EVE_FAULT_NONE;
static bool s_powerInhibit = false;

static void logLine(const char* line) {
  if (s_hooks.log) s_hooks.log(line);
}

void eveSafetyInit(const EveSafetyHooks* hooks) {
  if (hooks) s_hooks = *hooks;
  /* Deliberately does NOT clear a latched stop: init is only ever called once at boot. */
}

bool eveSafetyLatchStop(EveStopSource src) {
  const bool wasStopped = s_stopped;
  if (!wasStopped) {
    s_stopped = true;
    s_src = src;
    s_stopCount++;
    char buf[64];
    snprintf(buf, sizeof(buf), "[EVE][SAFETY] STOP latched (src=%s)", eveSafetyStopSourceName(src));
    logLine(buf);
  }
  /* Re-assert every time: a repeated STOP must still halt anything that slipped through. */
  if (s_hooks.haltOutputs) s_hooks.haltOutputs();
  return !wasStopped;
}

bool eveSafetyClearStop(EveStopSource src) {
  if (!s_stopped) return false;
  s_stopped = false;
  char buf[80];
  snprintf(buf, sizeof(buf), "[EVE][SAFETY] STOP cleared (src=%s, was %s) - motion stays idle until commanded",
           eveSafetyStopSourceName(src), eveSafetyStopSourceName(s_src));
  logLine(buf);
  s_src = EVE_STOP_SRC_NONE;
  return true;
}

bool eveSafetyStopLatched(void) { return s_stopped; }
EveStopSource eveSafetyStopSource(void) { return s_src; }
uint32_t eveSafetyStopCount(void) { return s_stopCount; }

const char* eveSafetyStopSourceName(EveStopSource src) {
  switch (src) {
    case EVE_STOP_SRC_WALLE: return "wall_e";
    case EVE_STOP_SRC_SERIAL: return "serial";
    case EVE_STOP_SRC_WEBUI: return "webui";
    case EVE_STOP_SRC_NONE:
    default: return "none";
  }
}

void eveSafetySetFault(uint16_t bits, bool active) {
  const uint16_t before = s_faults;
  if (active) {
    s_faults = (uint16_t)(s_faults | bits);
  } else {
    s_faults = (uint16_t)(s_faults & (uint16_t)~bits);
  }
  if (s_faults != before) {
    char buf[64];
    snprintf(buf, sizeof(buf), "[EVE][SAFETY] faults 0x%04x -> 0x%04x", (unsigned)before, (unsigned)s_faults);
    logLine(buf);
    if ((s_faults & EVE_FAULT_BLOCKING_MASK) && !(before & EVE_FAULT_BLOCKING_MASK) && s_hooks.haltOutputs) {
      s_hooks.haltOutputs();
    }
  }
}

uint16_t eveSafetyFaults(void) { return s_faults; }
bool eveSafetyHasBlockingFault(void) { return (s_faults & EVE_FAULT_BLOCKING_MASK) != 0; }

void eveSafetySetPowerInhibit(bool inhibit) {
  if (inhibit == s_powerInhibit) return;
  s_powerInhibit = inhibit;
  logLine(inhibit ? "[EVE][SAFETY] battery critical - motion inhibited" : "[EVE][SAFETY] battery recovered - motion allowed");
}

bool eveSafetyPowerInhibit(void) { return s_powerInhibit; }

bool eveSafetyMotionAllowed(void) { return !s_stopped && !eveSafetyHasBlockingFault() && !s_powerInhibit; }
bool eveSafetySoundAllowed(void) { return !s_stopped && !eveSafetyHasBlockingFault(); }
bool eveSafetyRoutinesAllowed(void) { return !s_stopped && !eveSafetyHasBlockingFault(); }
