#include "charging_control.h"
#include <stdio.h>

static bool s_chg_on = false;
static uint32_t s_charge_started_ms = 0;
static bool s_safety_locked = false;
static const char* s_safety_reason = "OK";
static float s_last_eve_pack_v = 0.0f;
static char s_pack_ov_msg[48];

static void safety_trip_(const char* reason) {
  if (!s_safety_locked) {
    Serial.print(F("[CHG][SAFETY] "));
    Serial.println(reason);
  }
  charging_emergency_off();
  s_safety_locked = true;
  s_safety_reason = reason;
}

static void safety_trip_pack_overvoltage_(void) {
  snprintf(s_pack_ov_msg, sizeof(s_pack_ov_msg), "EVE pack reached %.1fV - charge stopped", (double)DOCK_CHARGE_MAX_PACK_V);
  safety_trip_(s_pack_ov_msg);
}

int charging_init(void) {
  pinMode(DOCK_PIN_CHG_GATE, OUTPUT);
  digitalWrite(DOCK_PIN_CHG_GATE, DOCK_CHG_ACTIVE_HIGH ? LOW : HIGH);
  s_chg_on = false;
  s_charge_started_ms = 0;
  s_safety_locked = false;
  s_safety_reason = "OK";
  s_last_eve_pack_v = 0.0f;
#if DOCK_PIN_VBAT_SENSE >= 0
  pinMode(DOCK_PIN_VBAT_SENSE, INPUT);
  analogReadResolution(12);
#endif
  return 0;
}
void charging_set_enable(bool on) {
  if (!on) { charging_emergency_off(); return; }
  if (s_safety_locked) {
    charging_emergency_off();
    return;
  }
  if (DOCK_PIN_VBAT_SENSE >= 0 && !charging_voltage_ok_to_enable()) { charging_emergency_off(); return; }
  if (s_last_eve_pack_v >= DOCK_CHARGE_MAX_PACK_V) {
    safety_trip_pack_overvoltage_();
    return;
  }
  digitalWrite(DOCK_PIN_CHG_GATE, DOCK_CHG_ACTIVE_HIGH ? HIGH : LOW);
  if (!s_chg_on) {
    s_charge_started_ms = millis();
  }
  s_chg_on = true;
  Serial.println(F("[CHG] Charging path ENABLED"));
}
void charging_emergency_off(void) {
  digitalWrite(DOCK_PIN_CHG_GATE, DOCK_CHG_ACTIVE_HIGH ? LOW : HIGH);
  s_chg_on = false;
}
bool charging_is_enabled(void) { return s_chg_on; }
bool charging_voltage_ok_to_enable(void) {
#if DOCK_PIN_VBAT_SENSE < 0
  return true;
#else
  int v = analogRead(DOCK_PIN_VBAT_SENSE);
  return v >= 0 && (uint32_t)v >= DOCK_VBAT_SENSE_OK_MIN && (uint32_t)v <= DOCK_VBAT_SENSE_OK_MAX;
#endif
}

void charging_update(uint32_t now_ms) {
  if (!s_chg_on) return;
  if (s_last_eve_pack_v >= DOCK_CHARGE_MAX_PACK_V) {
    safety_trip_pack_overvoltage_();
    return;
  }
  if (s_charge_started_ms != 0 &&
      (uint32_t)(now_ms - s_charge_started_ms) >= DOCK_CHARGE_MAX_SESSION_MS) {
    safety_trip_("2 hour charge limit reached - charge stopped");
  }
}

void charging_note_eve_voltage(float pack_v) {
  if (pack_v <= 0.0f) return;
  s_last_eve_pack_v = pack_v;
  if (pack_v >= DOCK_CHARGE_MAX_PACK_V) {
    safety_trip_pack_overvoltage_();
  }
}

bool charging_is_safety_locked_out(void) { return s_safety_locked; }
const char* charging_safety_reason(void) { return s_safety_reason; }
uint32_t charging_session_elapsed_ms(uint32_t now_ms) {
  if (!s_chg_on || s_charge_started_ms == 0) return 0;
  return (uint32_t)(now_ms - s_charge_started_ms);
}
