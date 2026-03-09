/*******************************************************************************
 * Smart Charging Crate v1.1
 * WALL-E Docking Station - ESP32-WROOM-32 30-pin DevKit
 *
 * - ESP-NOW beacon at 10 Hz for WALL-E homing
 * - IR beam + optional obstacle sensors
 * - ACS712 current sensing (NOT_DOCKED, DOCKED_IDLE, CHARGING, CHARGED, FAULT)
 * - MOSFET charge enable
 * - NeoPixel status LEDs (non-blocking)
 ******************************************************************************/

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "dock_config.h"
#include "dock_protocol.h"
#include "dock_sensors.h"
#include "dock_neopixel.h"
#include "dock_espnow.h"
#include "dock_state.h"

/*=============================================================================
 * GLOBALS
 *===========================================================================*/

static uint32_t g_last_debug_ms = 0;

/*=============================================================================
 * SETUP
 *===========================================================================*/

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n[DOCK] Smart Charging Crate v1.1"));
  Serial.println(F("[DOCK] Booting..."));

  /* MOSFET: OFF by default (fail-safe) */
  pinMode(PIN_MOSFET_GATE, OUTPUT);
  digitalWrite(PIN_MOSFET_GATE, LOW);

  /* Sensors: calibrate ACS712 with charging OFF */
  Serial.println(F("[DOCK] Calibrating ACS712..."));
  dockSensorsBegin();
  Serial.print(F("[DOCK] ACS712 zero ADC: "));
  Serial.println(dockCurrentZero());

  /* NeoPixel */
  dockNeoPixelBegin();

  /* ESP-NOW (also connects to home WiFi if configured) */
  if (!dockEspNowBegin()) {
    Serial.println(F("[DOCK] ESP-NOW init FAILED"));
  } else {
    Serial.println(F("[DOCK] ESP-NOW init OK"));
  }

  /* OTA (after WiFi connected in dockEspNowBegin) */
  ArduinoOTA.setHostname("wall-e-dock");
  ArduinoOTA.onStart([]() { Serial.println(F("[OTA] Start")); });
  ArduinoOTA.onEnd([]() { Serial.println(F("\n[OTA] End")); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) { Serial.printf("[OTA] %u%%\r", (unsigned)(p * 100 / t)); });
  ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] Error %u\n", (unsigned)e); });
  ArduinoOTA.begin();
  Serial.println(F("[OTA] ArduinoOTA ready"));

  /* First sensor read */
  dockSensorsUpdate();
  Serial.print(F("[DOCK] Beam present: "));
  Serial.println(dockBeamPresent() ? F("YES") : F("NO"));
  Serial.print(F("[DOCK] Mouth blocked: "));
  Serial.println(dockMouthBlocked() ? F("YES") : F("NO"));
  Serial.print(F("[DOCK] Current raw: "));
  Serial.print(dockCurrentRaw());
  Serial.print(F(" amps: "));
  Serial.println(dockCurrentAmps(), 3);

  Serial.println(F("[DOCK] Entering loop"));
}

/*=============================================================================
 * LOOP
 *===========================================================================*/

void loop() {
  uint32_t now = millis();

  ArduinoOTA.handle();

  /* 1. Read sensors */
  dockSensorsUpdate();

  /* 2. Update state machine (controls MOSFET internally) */
  bool state_changed = dockStateUpdate();

  /* 3. Log state transition */
  if (state_changed) {
    DockState s = dockStateGet();
    Serial.print(F("[DOCK] State -> "));
    switch (s) {
      case STATE_BOOT:        Serial.println(F("BOOT")); break;
      case STATE_NOT_DOCKED:  Serial.println(F("NOT_DOCKED")); break;
      case STATE_DOCKED_IDLE: Serial.println(F("DOCKED_IDLE")); break;
      case STATE_CHARGING:    Serial.println(F("CHARGING")); break;
      case STATE_CHARGED:     Serial.println(F("CHARGED")); break;
      case STATE_FAULT:       Serial.println(F("FAULT")); break;
      default:                Serial.println(F("?")); break;
    }
    if (s == STATE_CHARGING && dockChargeEnabled()) {
      Serial.println(F("[DOCK] MOSFET ON"));
    }
    if (s == STATE_FAULT || s == STATE_NOT_DOCKED) {
      Serial.println(F("[DOCK] MOSFET OFF"));
    }
  }

  /* 4. Build and send ESP-NOW beacon */
  DockBeaconPacket_t pkt;
  pkt.magic = DOCK_BEACON_MAGIC;
  pkt.dock_id = DOCK_ID;
  pkt.uptime_ms = now;
  pkt.state = (uint8_t)dockStateGet();
  pkt.beam_present = dockBeamPresent() ? 1 : 0;
  pkt.mouth_blocked = dockMouthBlocked() ? 1 : 0;
  pkt.charge_enabled = dockChargeEnabled() ? 1 : 0;
  pkt.current_a_x100 = (int16_t)(dockCurrentAmps() * 100.0f);

  dockEspNowSendBeacon(&pkt);

  /* 5. Update NeoPixel */
  DockState s = dockStateGet();
  NeoPixelState np = (NeoPixelState)dockStateToNeoPixelState(s);
  bool mouth_warn = dockMouthBlocked() && (s == STATE_DOCKED_IDLE || s == STATE_CHARGING);
  dockNeoPixelUpdate(np, mouth_warn);

  /* 6. Periodic debug stats */
  if (now - g_last_debug_ms >= DEBUG_STATS_INTERVAL_MS) {
    g_last_debug_ms = now;

    uint32_t ok, fail;
    dockEspNowGetStats(&ok, &fail);

    Serial.print(F("[DOCK] ADC="));
    Serial.print(dockCurrentRaw());
    Serial.print(F(" I="));
    Serial.print(dockCurrentAmps(), 3);
    Serial.print(F("A beam="));
    Serial.print(dockBeamPresent() ? 1 : 0);
    Serial.print(F(" blocked="));
    Serial.print(dockMouthBlocked() ? 1 : 0);
    Serial.print(F(" chg="));
    Serial.print(dockChargeEnabled() ? 1 : 0);
    Serial.print(F(" ESPNOW ok="));
    Serial.print(ok);
    Serial.print(F(" fail="));
    Serial.println(fail);
  }
}
