#pragma once
#include <Arduino.h>
#include "pin_config.h"
int charging_init(void);
void charging_set_enable(bool on);
void charging_emergency_off(void);
bool charging_is_enabled(void);
bool charging_voltage_ok_to_enable(void);
