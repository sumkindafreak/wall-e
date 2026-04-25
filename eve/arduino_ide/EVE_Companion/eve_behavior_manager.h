#pragma once

#include <Arduino.h>
#include <stdint.h>

void eveBehaviorManagerInit(void);
void eveBehaviorManagerTick(void);

/** UART MSG_PLAY_SOUND from WALL-E — expression + optional local audio. */
void eveBehaviorOnRemoteSound(uint8_t track);
