#pragma once

#include <Arduino.h>

void eyesInit(void);
void eyesTick(void);
void eyesSetMode(uint8_t mode);

void eyesNotifyWallEConnected(void);
void eyesNotifyWallEDisconnected(void);
void eyesNotifyDockingState(bool docked, bool charging);
void eyesNotifyRecordFailure(void);
void eyesNotifySharedVoicebox(bool active);
