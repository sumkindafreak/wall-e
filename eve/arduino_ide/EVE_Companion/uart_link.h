#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include "eve_protocol.h"

/** Called once per valid frame (CRC and protocol version already checked). */
typedef void (*EveUartRxCallback)(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq);

void uartLinkInit(void);
/** Drain received bytes (bounded per call so stop handling stays responsive). */
void uartLinkPoll(void);
void uartLinkSetRxCallback(EveUartRxCallback cb);

/** Send JSON payload (UTF-8) as payload bytes; returns false if UART not ready or payload too large. */
bool uartLinkSendJson(EveMsgType type, const char* jsonUtf8);

uint8_t uartLinkNextSeq(void);
/** Total rejected frames (CRC + version + length errors). */
uint32_t uartLinkRxErrors(void);
uint32_t uartLinkFramesOk(void);
/** Detailed decoder counters for diagnostics. */
const EveFrameDecoder::Stats& uartLinkStats(void);
