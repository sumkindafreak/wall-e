#pragma once

#include <Arduino.h>
#include <stdint.h>

void audioInit(void);
void audioTick(void);

/** Legacy UART track index — resolved via SD /config/audio.json. */
void audioPlayTrack(uint8_t track);

void audioPlayPath(const char* sdRelativePath);
void audioQueuePath(const char* sdRelativePath);
void audioStop(void);
void audioPause(void);
void audioResume(void);
void audioSetVolume(uint8_t pct);
uint8_t audioGetVolume(void);
bool audioIsPlaying(void);
bool audioIsReady(void);
