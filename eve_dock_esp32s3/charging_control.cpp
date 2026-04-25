#include "charging_control.h"
static bool s_chg_on = false;
int charging_init(void) {
  pinMode(DOCK_PIN_CHG_GATE, OUTPUT);
  digitalWrite(DOCK_PIN_CHG_GATE, DOCK_CHG_ACTIVE_HIGH ? LOW : HIGH);
  s_chg_on = false;
#if DOCK_PIN_VBAT_SENSE >= 0
  pinMode(DOCK_PIN_VBAT_SENSE, INPUT);
  analogReadResolution(12);
#endif
  return 0;
}
void charging_set_enable(bool on) {
  if (!on) { charging_emergency_off(); return; }
  if (DOCK_PIN_VBAT_SENSE >= 0 && !charging_voltage_ok_to_enable()) { charging_emergency_off(); return; }
  digitalWrite(DOCK_PIN_CHG_GATE, DOCK_CHG_ACTIVE_HIGH ? HIGH : LOW);
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
