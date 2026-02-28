/***************************************************************
  IAN Audio Engine
  ESP32-audioI2S for MP3/WAV/AAC/FLAC playback from SD
  Same API as showduino IAN v1.0
***************************************************************/

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <stdint.h>
#include <SD.h>
#include "ian_config.h"

extern AudioState audioState;
extern uint8_t audioCurrentTrack;
extern bool audioLoopEnabled;
extern File audioFile;
extern uint32_t audioFileSize;
extern uint32_t audioBytesRead;
extern uint32_t lastAudioUpdateMs;
extern uint8_t audioBuffer[AUDIO_BUFFER_SIZE];

void audioInit();
void audioPlay(uint8_t trackIndex);
void audioStop();
void audioUpdate();
uint8_t audioGetTrackCount();
uint8_t audioGetProgress();

bool isAudioPlaying();
void setAudioVolume(uint8_t volume);
void playAudioFromSD(const char* filename);
void loopAudio();

void audioPlayTrack(uint8_t trackId);
void audioResume();
void audioPause();

uint8_t audioGetFileList(char* buffer, size_t bufferSize);

#endif
