#pragma once
#include <Arduino.h>
#include "pin_config.h"
int charging_init(void);
void charging_set_enable(bool on);
void charging_emergency_off(void);
bool charging_is_enabled(void);
bool charging_voltage_ok_to_enable(void);
void charging_update(uint32_t now_ms);
void charging_note_eve_voltage(float pack_v);
bool charging_is_safety_locked_out(void);
const char* charging_safety_reason(void);
uint32_t charging_session_elapsed_ms(uint32_t now_ms);
