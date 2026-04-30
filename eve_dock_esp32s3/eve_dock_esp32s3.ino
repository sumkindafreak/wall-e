#include "pin_config.h"

#if !DOCK_SAFE_BOOT_ONLY
#include "charging_control.h"
#include "dock_display.h"
#include "led_status.h"
#include "dock_control.h"
#endif

static uint32_t s_prn_ms = 0;

static void bootPrint(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.flush();
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1500);
  Serial.println();
  bootPrint(F("[BOOT] EVE dock sketch reached setup()"));
  bootPrint(F("[BOOT] Serial OK at 115200"));

#if DOCK_SAFE_BOOT_ONLY
  bootPrint(F("[BOOT] SAFE BOOT ONLY - dock hardware init is skipped"));
  bootPrint(F("[BOOT] If you see this, set DOCK_SAFE_BOOT_ONLY to 0 in pin_config.h"));
#else
  bootPrint(F("[BOOT] charging_init"));
  (void)charging_init();
  bootPrint(F("[BOOT] dockDisplayInit"));
  dockDisplayInit();
  bootPrint(F("[BOOT] led_init"));
  led_init();
  bootPrint(F("[BOOT] dock_init"));
  dock_init();
  bootPrint(F("[BOOT] setup complete - waiting for EVE on same pogo UART"));
#endif

  s_prn_ms = millis();
}

void loop() {
  uint32_t t = millis();

#if DOCK_SAFE_BOOT_ONLY
  if ((uint32_t)(t - s_prn_ms) > 1000u) {
    s_prn_ms = t;
    Serial.print(F("[SAFE] alive ms="));
    Serial.println((unsigned long)t);
    Serial.flush();
  }
#else
  dock_update(t);
  dockDisplayUpdateEx(t,
                      dock_get_state(),
                      charging_is_enabled(),
                      charging_voltage_ok_to_enable(),
                      dock_last_eve_rx_ms(),
                      charging_is_safety_locked_out(),
                      charging_safety_reason());

  if ((uint32_t)(t - s_prn_ms) > 2000u) {
    s_prn_ms = t;
    Serial.print(F("[STAT] fsm="));
    Serial.print(dock_state_name(dock_get_state()));
    Serial.print(F(" chg="));
    Serial.print(charging_is_enabled() ? F("on") : F("off"));
    if (charging_is_safety_locked_out()) {
      Serial.print(F(" safety="));
      Serial.print(charging_safety_reason());
    }
    Serial.print(F(" lastEveMs="));
    uint32_t lr = dock_last_eve_rx_ms();
    if (lr == 0) Serial.print(F("never"));
    else { Serial.print((unsigned long)(t - lr)); Serial.print(F(" ago")); }
    Serial.println();
  }
#endif

  yield();
}
