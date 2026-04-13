#ifndef AUDIO_ESP_BUTTON_MENU_MANAGER_H
#define AUDIO_ESP_BUTTON_MENU_MANAGER_H

#include <Arduino.h>
#include <stdint.h>
#include "menu_protocol.h"

void buttonMenuInit(void);
void buttonMenuTick(unsigned long now);

bool buttonMenuIsMenuMode(void);
uint8_t buttonMenuGetComboHoldPct(void);
walle_ui_event_t buttonMenuGetLastUiEvent(void);
void buttonMenuClearUiEvent(void);

#endif
