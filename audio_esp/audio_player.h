/**
 * audio_player.h — SD-card WAV + I2S playback for WALL-E audio node
 */
#ifndef AUDIO_ESP_AUDIO_PLAYER_H
#define AUDIO_ESP_AUDIO_PLAYER_H

#include "config.h"
#include "pins.h"
#include "debug_log.h"
#include <Arduino.h>

extern uint8_t g_audioPriority;

bool audioPlayerInit(void);
void audioPlayTrack(uint8_t trackId, uint8_t priority = PRIO_NORMAL);

void audioPlayStartup(void);
void audioPlayHello(void);
void audioPlayAck(void);
void audioPlayError(void);
void audioPlayCurious(void);
void audioPlaySleep(void);
void audioPlayWake(void);
void audioPlayDockGuide(void);
void audioPlayStop(void);

void audioStop(void);
void audioSetVolume(uint8_t vol);
bool audioIsReady(void);
bool audioIsBusy(void);
void audioPlayerTick(void);

#endif
