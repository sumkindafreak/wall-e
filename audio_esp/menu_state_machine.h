#ifndef AUDIO_ESP_MENU_STATE_MACHINE_H
#define AUDIO_ESP_MENU_STATE_MACHINE_H

#include <Arduino.h>
#include <stdint.h>
#include "menu_protocol.h"

void menuStateInit(void);
void menuStateResetTimeout(unsigned long now);
void menuStateTick(unsigned long now);

walle_menu_page_t menuStateGetPage(void);
uint8_t menuStateGetSel(void);

/** Menu button map: 1=up 2=down 3=select 4=back */
void menuStateOnButton(uint8_t menuBtn, unsigned long now);

#endif
