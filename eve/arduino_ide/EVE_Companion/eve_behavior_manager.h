#pragma once

#include <Arduino.h>
#include <stdint.h>

void eveBehaviorManagerInit(void);
void eveBehaviorManagerTick(void);

/**
 * UART MSG_PLAY_SOUND from WALL-E — eye reaction + local audio.
 * Returns true when the audio actually started (false: no audio hardware, file missing, stopped).
 */
bool eveBehaviorOnRemoteSound(uint8_t track);
