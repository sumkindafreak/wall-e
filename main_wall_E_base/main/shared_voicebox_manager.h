#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "menu_protocol.h"

void sharedVoiceboxInit(void);
void sharedVoiceboxTick(uint32_t nowMillis);

/** Called when a valid WAUI packet arrives from Audio ESP. */
void sharedVoiceboxOnAudioUi(const WalleAudioUiTelemPacket_t* pkt);

const char* sharedVoiceboxModeName(void);
bool sharedVoiceboxIsShared(void);
