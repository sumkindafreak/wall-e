/**
 * audio_esp_status.h — Store Audio ESP status from ESP-NOW (mic dir, dock IR, voice cmd, etc.)
 * Base can use this for dock alignment (when Audio ESP has dock IR) or display.
 */
#ifndef AUDIO_ESP_STATUS_H
#define AUDIO_ESP_STATUS_H

#include <stdint.h>
#include <stdbool.h>

/* Call when WalleAudioStatusPacket_t received */
void audioEspStatusOnPacket(const void* data, int len);

/* Getters — valid for ~3s after last packet */
bool audioEspStatusValid(void);
uint8_t audioEspStatusGetMicDir(void);
uint8_t audioEspStatusGetDockIr(void);
uint8_t audioEspStatusGetVoiceCmd(void);
uint8_t audioEspStatusGetMode(void);
uint8_t audioEspStatusGetFault(void);
uint8_t audioEspStatusGetAudioBusy(void);

/* Dock IR balance hint: negative=steer left, positive=steer right, 0=aligned */
int8_t audioEspStatusGetDockBalance(void);

#endif
