/**
 * audio_player.h — DFPlayer Mini control: play, stop, volume, busy state
 */
#ifndef AUDIO_ESP_AUDIO_PLAYER_H
#define AUDIO_ESP_AUDIO_PLAYER_H

#include "config.h"
#include "pins.h"
#include "debug_log.h"
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

/* DFPlayer UART */
extern HardwareSerial DFPlayerSerial;
extern DFRobotDFPlayerMini dfplayer;

/* Current playback priority (for interrupt logic) */
extern uint8_t g_audioPriority;

/* Initialize DFPlayer. Returns true on success. */
bool audioPlayerInit();

/* Play track by ID. priority: PRIO_* from config.h */
void audioPlayTrack(uint8_t trackId, uint8_t priority = PRIO_NORMAL);

/* Play by named event (maps to track IDs) */
void audioPlayStartup();
void audioPlayHello();
void audioPlayAck();
void audioPlayError();
void audioPlayCurious();
void audioPlaySleep();
void audioPlayWake();
void audioPlayDockGuide();
void audioPlayStop();

/* Stop playback. */
void audioStop();

/* Set volume 0–30 */
void audioSetVolume(uint8_t vol);

/* Returns true if DFPlayer initialized and ready */
bool audioIsReady();

/* Returns true if currently playing */
bool audioIsBusy();

/* Call every loop to update state (check for track end) */
void audioPlayerTick();

#endif
