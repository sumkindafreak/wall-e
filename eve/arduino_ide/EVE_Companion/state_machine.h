#pragma once

#include <Arduino.h>
#include <stdint.h>

void stateMachineInit(void);
void stateMachineTick(void);
void stateMachineOnUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq);

uint32_t stateMachineGetSessionId(void);
/** True when UART companion JSON may be sent (handshake done, not waiting / sleep / error). */
bool stateMachineAllowsCompanionUart(void);
/** Label advertised by the UART peer during ACK, e.g. "wall_e" or "eve_dock_c3". */
const char* stateMachineGetPeerLabel(void);
