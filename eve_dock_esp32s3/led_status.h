#pragma once
#include <Arduino.h>
#include "pin_config.h"
enum LedStatusMode : uint8_t { LED_MODE_OFF = 0, LED_MODE_BLUE_PULSE, LED_MODE_STANDBY, LED_MODE_GREEN_SOLID, LED_MODE_PURPLE_FLASH, LED_MODE_RED_BLINK };
void led_init(void);
void led_set_mode(LedStatusMode m);
void led_update(uint32_t now_ms);
void led_event_button_feedback(void);
LedStatusMode led_get_mode(void);
