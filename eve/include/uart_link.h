#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include "eve_protocol.h"

typedef void (*EveUartRxCallback)(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq);

void uartLinkInit(void);
void uartLinkPoll(void);
void uartLinkSetRxCallback(EveUartRxCallback cb);

/** Send JSON payload (UTF-8) as payload bytes; returns false if UART not ready or payload too large. */
bool uartLinkSendJson(EveMsgType type, const char* jsonUtf8);

uint8_t uartLinkNextSeq(void);
uint32_t uartLinkRxErrors(void);
uint32_t uartLinkFramesOk(void);
