#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "menu_protocol.h"

void audioUiTelemOnPacket(const uint8_t* data, int len);

bool audioUiTelemValid(void);
uint32_t audioUiTelemLastMs(void);
String audioUiTelemGetJSON(void);

uint8_t audioUiTelemGetBtnMode(void);
uint8_t audioUiTelemGetMenuPage(void);
uint8_t audioUiTelemGetComboPct(void);
uint8_t audioUiTelemGetLastEvent(void);
