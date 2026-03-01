/***************************************************************
  IAN Audio Engine — wall-e-heart
  Same implementation as showduino IAN v1.0
***************************************************************/

#include "audio_engine.h"
#include "ian_pins.h"
#include "ian_config.h"
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include "Audio.h"

AudioState audioState = AUDIO_STOPPED;
uint8_t audioCurrentTrack = 0;
bool audioLoopEnabled = false;
File audioFile;
uint32_t audioFileSize = 0;
uint32_t audioBytesRead = 0;
uint32_t lastAudioUpdateMs = 0;
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];

Audio audio;

static bool mp3Playing = false;
static char currentAudioFile[128] = "";
static bool audioPlaying = false;

void audioInit() {
  Serial.println(F("\n========================================"));
  Serial.println(F("[AUDIO] PCM5102A I2S AUDIO INITIALIZATION"));
  Serial.println(F("========================================"));

  Serial.print(F("[AUDIO] SD: CS=")); Serial.print(PIN_SD_CS);
  Serial.print(F(" MOSI=")); Serial.print(PIN_SD_MOSI);
  Serial.print(F(" MISO=")); Serial.print(PIN_SD_MISO);
  Serial.print(F(" SCK=")); Serial.println(PIN_SD_SCK);

  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  if (!SD.begin(PIN_SD_CS)) {
    Serial.println(F("[AUDIO] SD FAILED"));
    audioState = AUDIO_ERROR;
    return;
  }

  Serial.println(F("[AUDIO] SD OK"));
  Serial.print(F("[AUDIO] I2S: BCLK=")); Serial.print(PIN_I2S_BCLK);
  Serial.print(F(" LRCLK=")); Serial.print(PIN_I2S_LRCLK);
  Serial.print(F(" DATA=")); Serial.println(PIN_I2S_DATA);

  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRCLK, PIN_I2S_DATA);
  audio.setVolume(10);
  audio.setI2SCommFMT_LSB(false);

  audioState = AUDIO_STOPPED;
  audioCurrentTrack = 0;
  audioLoopEnabled = false;
  mp3Playing = false;
  currentAudioFile[0] = '\0';
  audioBytesRead = 0;
  audioFileSize = 0;
  lastAudioUpdateMs = 0;

  Serial.println(F("[AUDIO] ✓ Ready (MP3/WAV/AAC/FLAC)"));
  Serial.println(F("========================================\n"));
}

void audioPlay(uint8_t trackIndex) {
  if (trackIndex < 1) return;

  if (audioState == AUDIO_PLAYING || mp3Playing) audioStop();

  char filename[32];
  snprintf(filename, sizeof(filename), "/track%02d.wav", trackIndex);
  if (!SD.exists(filename)) snprintf(filename, sizeof(filename), "/track%02d.mp3", trackIndex);

  if (!SD.exists(filename)) {
    Serial.print(F("[AUDIO] File not found: ")); Serial.println(filename);
    audioState = AUDIO_ERROR;
    return;
  }

  strncpy(currentAudioFile, filename, sizeof(currentAudioFile) - 1);
  currentAudioFile[sizeof(currentAudioFile) - 1] = '\0';

  File f = SD.open(filename, FILE_READ);
  audioFileSize = f ? f.size() : 0;
  if (f) f.close();

  audio.connecttoFS(SD, filename);
  mp3Playing = true;
  audioPlaying = true;
  audioState = AUDIO_PLAYING;
  audioCurrentTrack = trackIndex;
  audioBytesRead = 0;

  Serial.print(F("[AUDIO] Play: ")); Serial.println(filename);
}

void audioStop() {
  if (audioState == AUDIO_STOPPED && !mp3Playing) return;

  if (mp3Playing) {
    audio.stopSong();
    mp3Playing = false;
  }

  audioState = AUDIO_STOPPED;
  audioCurrentTrack = 0;
  audioLoopEnabled = false;
  audioPlaying = false;
  currentAudioFile[0] = '\0';
  audioBytesRead = 0;
  audioFileSize = 0;

  Serial.println(F("[AUDIO] Stopped"));
}

void audioUpdate() {
  if (audioState != AUDIO_PLAYING && !mp3Playing) return;

  if (mp3Playing) {
    audio.loop();

    if (!audio.isRunning()) {
      if (audioLoopEnabled && audioCurrentTrack > 0) {
        char filename[32];
        snprintf(filename, sizeof(filename), "/track%02d.wav", audioCurrentTrack);
        if (!SD.exists(filename)) snprintf(filename, sizeof(filename), "/track%02d.mp3", audioCurrentTrack);
        if (SD.exists(filename)) {
          audio.connecttoFS(SD, filename);
          mp3Playing = true;
          audioPlaying = true;
          audioState = AUDIO_PLAYING;
          audioBytesRead = 0;
        } else {
          mp3Playing = false;
          audioPlaying = false;
          audioState = AUDIO_STOPPED;
          currentAudioFile[0] = '\0';
        }
      } else {
        mp3Playing = false;
        audioPlaying = false;
        audioState = AUDIO_STOPPED;
        currentAudioFile[0] = '\0';
      }
    }
  }
}

void loopAudio() { audioUpdate(); }

uint8_t audioGetTrackCount() {
  uint8_t count = 0;
  for (uint8_t i = 1; i <= 99; i++) {
    char fn[32];
    snprintf(fn, sizeof(fn), "/track%02d.wav", i);
    if (!SD.exists(fn)) snprintf(fn, sizeof(fn), "/track%02d.mp3", i);
    if (SD.exists(fn)) count = i; else break;
  }
  return count;
}

uint8_t audioGetProgress() { return 0; }

bool isAudioPlaying() {
  if (mp3Playing) return audio.isRunning();
  return (audioState == AUDIO_PLAYING);
}

void setAudioVolume(uint8_t volume) {
  if (volume > 100) volume = 100;
  uint8_t v = map(volume, 0, 100, 0, 21);
  audio.setVolume(v);
}

void playAudioFromSD(const char* filename) {
  if (!SD.exists(filename)) {
    Serial.print(F("[AUDIO] Not found: ")); Serial.println(filename);
    return;
  }

  if (mp3Playing) audioStop();

  strncpy(currentAudioFile, filename, sizeof(currentAudioFile) - 1);
  currentAudioFile[sizeof(currentAudioFile) - 1] = '\0';

  File f = SD.open(filename, FILE_READ);
  audioFileSize = f ? f.size() : 0;
  if (f) f.close();

  audio.connecttoFS(SD, filename);
  mp3Playing = true;
  audioPlaying = true;
  audioState = AUDIO_PLAYING;
  audioBytesRead = 0;

  if (strncmp(filename, "/track", 6) == 0) audioCurrentTrack = atoi(filename + 6);
  else audioCurrentTrack = 0;

  Serial.print(F("[AUDIO] Play: ")); Serial.println(filename);
}

uint8_t audioGetFileList(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) return 0;
  buffer[0] = '\0';
  uint8_t count = 0;
  size_t used = 0;
  for (uint8_t i = 1; i <= 99 && used < bufferSize - 20; i++) {
    char fnWav[32], fnMp3[32];
    snprintf(fnWav, sizeof(fnWav), "/track%02d.wav", i);
    snprintf(fnMp3, sizeof(fnMp3), "/track%02d.mp3", i);
    const char* found = SD.exists(fnWav) ? fnWav : (SD.exists(fnMp3) ? fnMp3 : nullptr);
    if (found) {
      if (count) buffer[used++] = '|';
      const char* name = found + 1;
      size_t len = strlen(name);
      if (used + len < bufferSize - 1) {
        strncpy(buffer + used, name, bufferSize - used - 1);
        used += len;
        buffer[used] = '\0';
        count++;
      } else break;
    } else break;
  }
  return count;
}

void audioPlayTrack(uint8_t trackId) { audioPlay(trackId); }

void audioResume() {
  if (audioState == AUDIO_PAUSED && strlen(currentAudioFile) > 0) {
    audioState = AUDIO_PLAYING;
    audio.connecttoFS(SD, currentAudioFile);
    mp3Playing = true;
  }
}

void audioPause() {
  if (audioState == AUDIO_PLAYING) {
    audioState = AUDIO_PAUSED;
    audio.stopSong();
    mp3Playing = false;
  }
}
