/**
 * WALL-E / EVE — native SD-card WAV decoder + I2S output.
 * Non-blocking playback with queue, volume, pause/resume, and event callbacks.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

typedef enum {
  WALLE_AUDIO_EVT_STARTED = 0,
  WALLE_AUDIO_EVT_STOPPED,
  WALLE_AUDIO_EVT_PAUSED,
  WALLE_AUDIO_EVT_RESUMED,
  WALLE_AUDIO_EVT_COMPLETED,
  WALLE_AUDIO_EVT_ERROR,
  WALLE_AUDIO_EVT_QUEUE_EMPTY,
} WalleAudioEvent;

typedef enum {
  WALLE_AUDIO_IDLE = 0,
  WALLE_AUDIO_PLAYING,
  WALLE_AUDIO_PAUSED,
  WALLE_AUDIO_ERROR,
} WalleAudioStatus;

typedef struct {
  int8_t bclkPin;
  int8_t lrckPin;
  int8_t doutPin;
  uint8_t portIndex; /* 0 = I2S_NUM_0, 1 = I2S_NUM_1 */
} WalleI2sPinConfig;

typedef void (*WalleAudioEventFn)(WalleAudioEvent evt, const char* path);

/** Initialize I2S TX. Returns true when pins are valid and driver installed. */
bool walleI2sAudioInit(const WalleI2sPinConfig* pins);

void walleI2sAudioSetEventCallback(WalleAudioEventFn fn);

/** Must be called every loop — pumps PCM to I2S and advances queue. */
void walleI2sAudioTick(void);

bool walleI2sAudioIsReady(void);
WalleAudioStatus walleI2sAudioGetStatus(void);
bool walleI2sAudioIsPlaying(void);

/** Play immediately (clears queue). Path is absolute or SD-root relative. */
bool walleI2sAudioPlayFile(const char* path);

/** Enqueue for playback after current track finishes. */
bool walleI2sAudioQueueFile(const char* path);

void walleI2sAudioStop(void);
void walleI2sAudioPause(void);
void walleI2sAudioResume(void);

/** Volume 0–100 (software scale on PCM). */
void walleI2sAudioSetVolume(uint8_t pct);
uint8_t walleI2sAudioGetVolume(void);

void walleI2sAudioClearQueue(void);
uint8_t walleI2sAudioQueueDepth(void);
