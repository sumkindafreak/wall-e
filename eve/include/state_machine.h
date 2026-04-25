#pragma once

#include <Arduino.h>
#include <stdint.h>

void stateMachineInit(void);
void stateMachineTick(void);
void stateMachineOnUartRx(uint8_t type, const uint8_t* payload, size_t len, uint8_t seq);

uint32_t stateMachineGetSessionId(void);
bool stateMachineAllowsCompanionUart(void);
const char* stateMachineGetPeerLabel(void);
