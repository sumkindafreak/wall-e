#pragma once
#include <stdint.h>

void audioEspNowInit(void);
bool audioEspNowPlayTrack(uint8_t track, uint8_t priority);
bool audioEspNowSetVolume(uint8_t vol_dfplayer_0_30);
bool audioEspNowSendEvent(uint8_t event_id, uint8_t priority);
bool audioEspNowStop(void);

/** Voicebox personality command to Audio ESP (ESP-NOW). */
bool audioEspNowSendVoiceboxCmd(uint8_t mode, uint8_t eve_audio_ok, uint8_t bond_strength);
