#include "config.h"

#if EVE_ENABLE_AWARENESS

#include "eve_awareness.h"
#include "battery_monitor.h"
#include "state_machine.h"
#include "system_status.h"
#include "audio_control.h"
#include "eve_asset_manager.h"
#include "mic_input.h"

#include <Arduino.h>
#include <string.h>

static EveAwarenessSnapshot s_snap;
static uint32_t s_lastSerialMs = 0;

static bool peerLinked(void) {
  const char* peer = stateMachineGetPeerLabel();
  return peer && peer[0] != '\0' && strcmp(peer, "none") != 0;
}

static void publishPersonFacts(void) {
  /* O-2: ToF publisher updates these fields. O-1 leaves neutral defaults. */
  s_snap.personPresent = false;
  s_snap.personDistanceMm = -1.f;
  s_snap.personZone = 0;
  s_snap.personConfidence = 0;
}

static void publishBatteryFacts(void) {
  s_snap.batteryVoltage = eveBatteryDataValid() ? eveBatteryVoltage() : 0.f;
  s_snap.batteryLow = eveBatteryIsCritical() || eveBatteryStatus() == EVE_BAT_WARN ||
                      eveBatteryStatus() == EVE_BAT_CRITICAL;
  s_snap.charging =
      stateMachineIsDocked() && eveBatteryDataValid() && eveBatteryCurrentA() > 0.04f;
}

static void publishDockFacts(void) {
  s_snap.docked = stateMachineIsDocked();
}

static void publishLinkFacts(void) {
  s_snap.wallELinked = stateMachineGetSessionId() != 0 && peerLinked();
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
  s_snap.displayReady = false; /* O-1: face ready flag comes with eye bench API later */
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
  publishDockFacts();
  publishLinkFacts();
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
  Serial.println(s_snap.personZone);
  Serial.print(F("Confidence...."));
  Serial.println(s_snap.personConfidence);
  Serial.print(F("Battery......."));
  Serial.print(s_snap.batteryVoltage, 2);
  Serial.println(F("V"));
  Serial.print(F("Battery low..."));
  printYesNo(s_snap.batteryLow);
  Serial.print(F("Charging......"));
  printYesNo(s_snap.charging);
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
