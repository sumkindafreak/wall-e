#pragma once

#include <Arduino.h>

/**
 * Optional UART link to EVE companion (same framing as eve/uart_link + eve_protocol).
 * WALL-E base pins: TX to EVE RX (GPIO18), RX from EVE TX (GPIO17) — matches eve/config.h crossover.
 */
#ifndef EVE_BRIDGE_UART_TX
#define EVE_BRIDGE_UART_TX 18
#endif
#ifndef EVE_BRIDGE_UART_RX
#define EVE_BRIDGE_UART_RX 17
#endif
#ifndef EVE_BRIDGE_BAUD
#define EVE_BRIDGE_BAUD 115200
#endif

void eveUartBridgeInit(void);
void eveUartBridgePoll(void);

String eveUartBridgeGetJSON(void);

/** True if a valid frame was received within timeout (same window as JSON link_ok). */
bool eveUartBridgeIsLinkUp(void);

/** Send MSG_PLAY_SOUND with DFPlayer track index on EVE (non-blocking UART write). */
bool eveUartBridgeSendPlaySound(uint8_t track);

/** Handshake: optional session echo for EVE state machine. */
bool eveUartBridgeSendWallEAck(uint32_t session);
