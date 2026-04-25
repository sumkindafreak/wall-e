#include "pin_config.h"
#include "charging_control.h"
#include "dock_display.h"
#include "led_status.h"
#include "dock_control.h"

static uint32_t s_prn_ms = 0;

static void bootLog(const __FlashStringHelper* msg) {
  Serial.println(msg);
  Serial.flush();
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  uint32_t serialWaitStart = millis();
  while (!Serial && (uint32_t)(millis() - serialWaitStart) < 3000u) {
    delay(10);
  }
  delay(500);
  Serial.println();
  bootLog(F("[BOOT] EVE Dock/Base Controller starting"));
  bootLog(F("[BOOT] If you only see ESP-ROM, enable USB CDC On Boot or check board/port"));

  bootLog(F("[BOOT] charging_init"));
  (void)charging_init();
  bootLog(F("[BOOT] dockDisplayInit"));
  dockDisplayInit();
  bootLog(F("[BOOT] led_init"));
  led_init();
  bootLog(F("[BOOT] dock_init"));
  dock_init();
  bootLog(F("[BOOT] setup complete - waiting for EVE on same pogo UART"));
  s_prn_ms = millis();
}

void loop() {
  uint32_t t = millis();
  dock_update(t);
  dockDisplayUpdate(t, dock_get_state(), charging_is_enabled(), dock_last_eve_rx_ms());
  yield();

  if ((uint32_t)(t - s_prn_ms) > 2000u) {
    s_prn_ms = t;
    Serial.print(F("[STAT] fsm="));
    Serial.print(dock_state_name(dock_get_state()));
    Serial.print(F(" chg="));
    Serial.print(charging_is_enabled() ? F("on") : F("off"));
    Serial.print(F(" lastEveMs="));
    uint32_t lr = dock_last_eve_rx_ms();
    if (lr == 0) Serial.print(F("never"));
    else { Serial.print((unsigned long)(t - lr)); Serial.print(F(" ago")); }
    Serial.println();
  }
}
