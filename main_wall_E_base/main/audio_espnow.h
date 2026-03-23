#pragma once
#include <stdint.h>

void audioEspNowInit(void);
bool audioEspNowPlayTrack(uint8_t track, uint8_t priority);
bool audioEspNowSetVolume(uint8_t vol_dfplayer_0_30);
bool audioEspNowSendEvent(uint8_t event_id, uint8_t priority);
bool audioEspNowStop(void);
