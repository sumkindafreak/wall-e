#pragma once

// ============================================================
//  WALL-E Flashlight (LDR + MOSFET)
//  When ambient light is dark, turn MOSFET on → LED flashlight on.
// ============================================================

#include <Arduino.h>

// --- Pins ---
#define LDR_ADC_PIN      3   // ADC pin: LDR in voltage divider to GND (see comment below)
#define FLASHLIGHT_PIN  10   // GPIO to MOSFET gate — digital on/off only (HIGH = on, LOW = off), no PWM

// --- LDR circuit ---
// Set LDR_DARK_WHEN_LOW depending on wiring:
//   0 = 3.3V — R — ADC — LDR — GND  → dark = HIGH raw (turn light on when raw >= LDR_DARK_RAW)
//   1 = 3.3V — LDR — ADC — R — GND  → dark = LOW  raw (turn light on when raw <= LDR_DARK_RAW)
#define LDR_DARK_WHEN_LOW  0

#define LDR_SAMPLES      8
#define LDR_POLL_MS    500   // check every 500 ms
#define LDR_DARK_RAW  1800   // threshold (0–4095): dark = above this if DARK_WHEN_LOW=0, below if =1
#define LDR_HYST       150   // hysteresis to avoid flicker
// Min on/off time (ms) so flashlight doesn't oscillate if its light hits the LDR
#define LDR_MIN_ON_MS  2000
#define LDR_MIN_OFF_MS 2000

void flashlightInit();
void flashlightHandle();   // call in loop()
