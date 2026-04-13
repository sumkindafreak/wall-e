#pragma once

#include <Arduino.h>
#include <stdint.h>

/** Thin facade over UART EVE bridge for future attach/dock fusion. */
void eveLinkManagerInit(void);
void eveLinkManagerTick(uint32_t nowMillis);

bool eveLinkIsEveUartUp(void);
