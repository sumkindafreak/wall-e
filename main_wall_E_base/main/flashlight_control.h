#pragma once

// ============================================================
// WALL-E Flashlight (LDR + MOSFET)
// ============================================================

#include <Arduino.h>
#include "base_board_pins.h"

#define LDR_ADC_PIN      BASE_PIN_LDR_ADC
#define FLASHLIGHT_PIN   BASE_PIN_FLASHLIGHT

#define LDR_DARK_WHEN_LOW  0

#define LDR_SAMPLES      8
#define LDR_POLL_MS      500
#define LDR_DARK_RAW     1800
#define LDR_HYST         150
#define LDR_MIN_ON_MS    2000
#define LDR_MIN_OFF_MS   2000

void flashlightInit();
void flashlightHandle();
