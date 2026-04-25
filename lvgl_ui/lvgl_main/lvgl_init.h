#pragma once

#include <Arduino.h>
#include <lvgl.h>

// LVGL + display bring-up with TFT_eSPI.
void lvglPlatformInit(void);

// Call each loop (non-blocking).
void lvglPlatformTask(void);
