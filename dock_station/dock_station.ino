/*******************************************************************************
 * Smart Charging Crate v1.1
 * WALL-E Docking Station - ESP32-S3 DevKit-style board (e.g. ESP32-S3 N16R8)
 *
 * - ESP-NOW beacon at 10 Hz for WALL-E homing
 * - IR beam + optional obstacle sensors
 * - ACS712 current sensing (NOT_DOCKED, DOCKED_IDLE, CHARGING, CHARGED, FAULT)
 * - MOSFET charge enable
 * - NeoPixel status LEDs (non-blocking)
 ******************************************************************************/

#include <Arduino.h>
#if ENABLE_WIFI
#include <ArduinoOTA.h>
#include <WiFi.h>
#endif
#include "dock_config.h"
#include "dock_protocol.h"
#include "dock_sensors.h"
#if ENABLE_WIFI
#include "dock_espnow.h"
#endif
#include "dock_state.h"
#include "dock_alignment.h"
#include "dock_callout.h"
#include "dock_display.h"
#include "dock_hw.h"
#include "dock_sonar.h"
#include "dock_vl6180.h"
#include "dock_test.h"
#include "dock_neopixel.h"

/*=============================================================================
 * GLOBALS
 *===========================================================================*/

static uint32_t g_last_debug_ms = 0;
static bool g_espnow_ok = false;

/*=============================================================================
 * SETUP
 *===========================================================================*/

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n[DOCK] Smart Charging Crate v1.1"));
  Serial.println(F("[DOCK] Booting..."));

  /* MOSFET gate and internal LED via dock_hw helpers (pin validity checks) */
  dockConfigureOutputPin(PIN_MOSFET_GATE, LOW, "charge gate");
  dockConfigureOutputPin(PIN_INTERNAL_LED,
                         MOSFET_INTERNAL_ACTIVE_LOW ? LOW : HIGH,
                         "internal LED");

  /* Sensors (IR + obstacles, current sense; sonar optional last-resort) */
  Serial.println(F("[DOCK] Enabling sensors (no current sense on S3)"));
  dockSensorsBegin();
#if USE_VL6180_TOF
  dockVl6180Begin();
#endif
#if USE_SONAR
  dockSonarBegin();
  Serial.println(F("[DOCK] Sonar enabled (last-resort dock detection)"));
#endif

  /* Alignment + arrow MOSFETs */
  Serial.println(F("[DOCK] Enabling alignment arrows"));
  dockAlignmentBegin();

  /* Call WALL-E switch + light show */
  Serial.println(F("[DOCK] Enabling callout switch + light show"));
  dockCalloutBegin();

  /* NeoPixel strip on PIN_STATUS_NEOPIXEL (GPIO2) */
  Serial.println(F("[DOCK] Enabling NeoPixel status strip"));
  dockNeoPixelBegin();
  /* Dock TFT display */
  Serial.println(F("[DOCK] Enabling TFT display (ST7789)"));
  dockDisplayBegin();

#if ENABLE_WIFI
  /* WiFi: start connection (non-blocking). ESP-NOW inited from loop when WiFi connects. */
  g_espnow_ok = dockEspNowBegin();
  if (!g_espnow_ok) {
    Serial.println(F("[DOCK] No WiFi credentials (set WIFI_HOME_SSID or use Share from WALL-E)"));
  }
#else
  Serial.println(F("[DOCK] WiFi disabled (ENABLE_WIFI=0 in dock_config.h)"));
#endif

#if DOCK_TEST_MODE
  Serial.println(F("[DOCK] Running test sequence (DOCK_TEST_MODE=1)..."));
  dockTestRun();
  Serial.println(F("[DOCK] Test done. Entering normal loop."));
#endif
}

/*=============================================================================
 * LOOP
 *===========================================================================*/

void loop() {
  static uint32_t last_log_ms = 0;
  uint32_t now = millis();

#if DOCK_TEST_SERIAL_CMD
  {
    int cmd = dockTestCheckSerial();
    if (cmd) dockTestExecuteCommand(cmd);
  }
#endif

#if ENABLE_WIFI
  /* Finish WiFi/ESP-NOW init when connected (non-blocking) */
  g_espnow_ok = dockEspNowPoll();
  if (g_espnow_ok && WiFi.status() == WL_CONNECTED) {
    static bool ota_started = false;
    if (!ota_started) {
      ota_started = true;
      ArduinoOTA.setHostname("wall-e-dock");
      ArduinoOTA.begin();
      Serial.println(F("[DOCK] OTA enabled (hostname wall-e-dock, port 3232)"));
    }
  }
  ArduinoOTA.handle();
  if (g_espnow_ok) {
    DockBeaconPacket_t pkt = {};
    pkt.magic = DOCK_BEACON_MAGIC;
    pkt.dock_id = DOCK_ID;
    pkt.uptime_ms = (uint32_t)millis();
    pkt.state = (uint8_t)dockStateGet();
    pkt.beam_present = dockDockDetected() ? 1 : 0;
    pkt.mouth_blocked = dockMouthBlocked() ? 1 : 0;
    pkt.charge_enabled = dockChargeEnabled() ? 1 : 0;
    pkt.callout_active = dockCalloutIsActive() ? 1 : 0;
    pkt.current_a_x100 = (int16_t)(dockCurrentAmps() * 100.0f);
    dockEspNowSendBeacon(&pkt);
  }
#endif

  /* 1. Read sensors */
  dockSensorsUpdate();

  /* 2. Update state machine (controls MOSFET internally) */
  bool state_changed = dockStateUpdate();

  /* 3. Callout (push button) and alignment arrows */
  dockCalloutUpdate();
  if (!dockCalloutIsActive()) {
    dockAlignmentUpdate(dockDockDetected());
  }

  /* 4. TFT: state, current, dock, mouth (rate-limited inside dockDisplayUpdate) */
  dockDisplayUpdate();

  /* 5. NeoPixel status strip */
  {
    DockState s = dockStateGet();
    NeoPixelState np = dockCalloutIsActive()
      ? NP_STATE_CALLOUT
      : (NeoPixelState)dockStateToNeoPixelState(s);
    bool mouth_warn = dockMouthBlocked() && (s == STATE_DOCKED_IDLE || s == STATE_CHARGING);
    dockNeoPixelUpdateEx(np, mouth_warn, dockStateGetFaultCode(),
                         dockDockDetected(), dockMouthBlocked(), dockCurrentAmps());
  }

  /* 6. Log state transition */
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
  }

  /* 7. Periodic simple debug every 2 seconds */
  if (now - last_log_ms >= 2000) {
    last_log_ms = now;
    Serial.print(F("[DOCK] Beam="));
    Serial.print(dockBeamPresent() ? 1 : 0);
    Serial.print(F(" mouth="));
    Serial.print(dockMouthBlocked() ? 1 : 0);
    Serial.print(F(" state="));
    Serial.println((int)dockStateGet());
  }
}
