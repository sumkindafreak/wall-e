#include "config.h"

#if EVE_ENABLE_AWARENESS

#include "eve_awareness.h"
#include "eve_awareness_zones.h"
#include "eve_awareness_battery.h"
#include "eve_awareness_connection.h"
#include "battery_monitor.h"
#include "state_machine.h"
#include "system_status.h"
#include "audio_control.h"
#include "eve_asset_manager.h"
#include "mic_input.h"

#if EVE_ENABLE_TOF
#include "eve_tof_manager.h"
#endif

#include <Arduino.h>
#include <limits.h>
#include <string.h>

static EveAwarenessSnapshot s_snap;
static uint32_t s_lastSerialMs = 0;

#if EVE_ENABLE_TOF
static int32_t tofClosestDistanceMm(const EveTofRawFrame* raw) {
  int32_t best = INT32_MAX;
  bool any = false;

  if ((raw->valid_mask & 1u) != 0u && raw->left_mm > 0) {
    best = raw->left_mm;
    any = true;
  }
  if ((raw->valid_mask & 2u) != 0u && raw->right_mm > 0) {
    if (!any || raw->right_mm < best) {
      best = raw->right_mm;
    }
    any = true;
  }
  if ((raw->valid_mask & 4u) != 0u && raw->center_mm > 0) {
    if (!any || raw->center_mm < best) {
      best = raw->center_mm;
    }
    any = true;
  }

  return any ? best : -1;
}
#endif

static void publishPersonFacts(void) {
#if EVE_ENABLE_TOF
  EveTofRawFrame raw;
  eveTofManagerGetLastFrame(&raw);
  const int32_t distMm = tofClosestDistanceMm(&raw);

  if (distMm > 0 && distMm < EVE_TOF_FAR_IGNORE_MM) {
    s_snap.personDistanceMm = (float)distMm;
    s_snap.personZone =
        eveAwarenessZoneFromDistanceMm(distMm, EVE_TOF_FAR_IGNORE_MM);
    s_snap.personConfidence = 100;
  } else {
    s_snap.personDistanceMm = -1.f;
    s_snap.personZone = EVE_AWARENESS_ZONE_UNKNOWN;
    s_snap.personConfidence = 0;
  }

  s_snap.personPresent =
      s_snap.personConfidence >= EVE_AWARENESS_PERSON_PRESENT_THRESHOLD;
#else
  s_snap.personPresent = false;
  s_snap.personDistanceMm = -1.f;
  s_snap.personZone = EVE_AWARENESS_ZONE_UNKNOWN;
  s_snap.personConfidence = 0;
#endif
}

static void publishBatteryFacts(void) {
#if EVE_ENABLE_BATTERY_MONITOR
  const bool valid = eveBatteryDataValid();
  if (valid) {
    s_snap.batteryVoltage = eveBatteryVoltage();
    s_snap.batteryPercent = (int8_t)eveBatteryPercent();
    s_snap.batteryHealth =
        eveAwarenessBatteryHealthFromStatus((int)eveBatteryStatus(), true);
  } else {
    s_snap.batteryVoltage = 0.f;
    s_snap.batteryPercent = -1;
    s_snap.batteryHealth = EVE_AWARENESS_BATTERY_HEALTH_UNKNOWN;
  }

  s_snap.charging = eveAwarenessChargingFromCurrentA(eveBatteryCurrentA(), valid,
                                                     EVE_AWARENESS_CHARGING_MIN_A);
  s_snap.batteryLow =
      eveAwarenessBatteryLowFromPercent(s_snap.batteryPercent, EVE_AWARENESS_BATTERY_LOW_PCT);
#else
  s_snap.batteryVoltage = 0.f;
  s_snap.batteryPercent = -1;
  s_snap.batteryLow = false;
  s_snap.charging = false;
  s_snap.batteryHealth = EVE_AWARENESS_BATTERY_HEALTH_UNKNOWN;
#endif
}

static void publishConnectionFacts(void) {
  const uint32_t sessionId = stateMachineGetSessionId();
  const bool peerLinked =
      eveAwarenessPeerLabelIsLinked(stateMachineGetPeerLabel());

  s_snap.docked = stateMachineIsDocked();
  s_snap.wallELinked = eveAwarenessWallELinkedFromSession(sessionId, peerLinked);
  s_snap.remoteConnected = stateMachineAllowsCompanionUart();
}

static void publishAudioFacts(void) {
  s_snap.audioPlaying = audioIsPlaying();
  s_snap.audioReady = audioIsReady();
  if (isMicConfigured() && isMicRunning()) {
    s_snap.voiceDetected = getMicLevel() > QUIET_THRESHOLD;
  } else {
    s_snap.voiceDetected = false;
  }
}

static void publishHealthFacts(void) {
  s_snap.sdMounted = eveAssetIsMounted();
#if EVE_ENABLE_EYES
  s_snap.displayReady = false; /* O-5: face ready flag when eye bench API exists */
#else
  s_snap.displayReady = true;
#endif
}

void eveAwarenessInit(void) {
  memset(&s_snap, 0, sizeof(s_snap));
  s_lastSerialMs = 0;
  Serial.println(F("[EVE][AWARE] init"));
}

void eveAwarenessTick(void) {
  publishPersonFacts();
  publishBatteryFacts();
  publishConnectionFacts();
  publishAudioFacts();
  publishHealthFacts();
  s_snap.uptimeMs = systemStatusUptimeMs();

#if EVE_AWARENESS_SERIAL_DEBUG
  uint32_t now = millis();
  if (s_lastSerialMs == 0 || (now - s_lastSerialMs) >= EVE_AWARENESS_SERIAL_INTERVAL_MS) {
    s_lastSerialMs = now;
    eveAwarenessPrintSerial();
  }
#endif
}

const EveAwarenessSnapshot& eveAwarenessGetSnapshot(void) {
  return s_snap;
}

static void printYesNo(bool v) {
  Serial.println(v ? F("YES") : F("NO"));
}

void eveAwarenessPrintSerial(void) {
  Serial.println(F("-----------------------------------"));
  Serial.println(F("AWARENESS"));
  Serial.print(F("Person........"));
  printYesNo(s_snap.personPresent);
  Serial.print(F("Distance......"));
  if (s_snap.personDistanceMm >= 0.f) {
    Serial.print((int)s_snap.personDistanceMm);
    Serial.println(F("mm"));
  } else {
    Serial.println(F("—"));
  }
  Serial.print(F("Zone.........."));
  Serial.println(eveAwarenessZoneName(s_snap.personZone));
  Serial.print(F("Confidence...."));
  Serial.println(s_snap.personConfidence);
  Serial.print(F("Battery V....."));
  Serial.print(s_snap.batteryVoltage, 2);
  Serial.println(F("V"));
  Serial.print(F("Battery %....."));
  if (s_snap.batteryPercent >= 0) {
    Serial.println(s_snap.batteryPercent);
  } else {
    Serial.println(F("—"));
  }
  Serial.print(F("Battery low..."));
  printYesNo(s_snap.batteryLow);
  Serial.print(F("Charging......"));
  printYesNo(s_snap.charging);
  Serial.print(F("Batt health..."));
  Serial.println(eveAwarenessBatteryHealthName(s_snap.batteryHealth));
  Serial.print(F("Docked........"));
  printYesNo(s_snap.docked);
  Serial.print(F("WALL-E link..."));
  printYesNo(s_snap.wallELinked);
  Serial.print(F("Remote........"));
  printYesNo(s_snap.remoteConnected);
  Serial.print(F("Voice........."));
  printYesNo(s_snap.voiceDetected);
  Serial.print(F("Audio........."));
  printYesNo(s_snap.audioPlaying);
  Serial.print(F("SD............"));
  Serial.println(s_snap.sdMounted ? F("OK") : F("—"));
  Serial.print(F("Display......."));
  Serial.println(s_snap.displayReady ? F("OK") : F("—"));
  Serial.print(F("Uptime........"));
  Serial.print(s_snap.uptimeMs / 1000u);
  Serial.println(F("s"));
  Serial.println(F("-----------------------------------"));
}

#else /* !EVE_ENABLE_AWARENESS */

#include "eve_awareness.h"

static EveAwarenessSnapshot s_stub;

void eveAwarenessInit(void) {}
void eveAwarenessTick(void) {}
const EveAwarenessSnapshot& eveAwarenessGetSnapshot(void) {
  return s_stub;
}
void eveAwarenessPrintSerial(void) {}

#endif /* EVE_ENABLE_AWARENESS */
