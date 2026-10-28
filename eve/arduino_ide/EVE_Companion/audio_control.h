/**
 * EVE audio — SD card WAV files played over I2S (the engine this build already uses).
 *
 * - Track numbers map to files through the SD card's /config/audio.json, falling back to
 *   /audio/NNN.wav. This firmware ships NO recordings: a missing file is logged and skipped,
 *   and EVE carries on without sound.
 * - New sounds are refused while EVE is stopped or has a blocking fault.
 * - audioStop() is the explicit stop operation (called by every STOP).
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

void audioInit(void);
void audioTick(void);

/** Legacy UART track index — resolved via SD /config/audio.json. Returns true if playback started. */
bool audioPlayTrack(uint8_t track);

bool audioPlayPath(const char* sdRelativePath);
bool audioQueuePath(const char* sdRelativePath);
void audioStop(void);
void audioPause(void);
void audioResume(void);
void audioSetVolume(uint8_t pct);
uint8_t audioGetVolume(void);
bool audioIsPlaying(void);
/** I2S running AND SD card mounted. */
bool audioIsReady(void);
/** Number of play requests that failed (missing file, not ready, refused). */
uint32_t audioFailCount(void);
