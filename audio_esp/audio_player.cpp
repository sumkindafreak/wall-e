/**
 * audio_player.cpp — SD-card WAV + I2S implementation
 */
#include "audio_player.h"
#include "walle_i2s_wav_player.h"

#include <SD.h>
#include <SPI.h>
#include <stdio.h>

uint8_t g_audioPriority = 0;
static bool s_sdOk = false;
static uint8_t s_playPriority = 0;

static bool mountStorage(void) {
  if (s_sdOk) {
    return true;
  }
  if (PIN_SD_CS < 0) {
    return false;
  }
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  s_sdOk = SD.begin(PIN_SD_CS, SPI, 20000000);
  return s_sdOk;
}

static bool trackPath(uint8_t trackId, char* out, size_t outLen) {
  if (!out || outLen == 0) {
    return false;
  }
  int n = snprintf(out, outLen, "/audio/%03u.wav", (unsigned)trackId);
  return n > 0 && (size_t)n < outLen;
}

bool audioPlayerInit(void) {
  g_audioPriority = 0;
  s_playPriority = 0;
  mountStorage();

  WalleI2sPinConfig pins = {PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT, I2S_AUDIO_PORT_INDEX};
  if (!walleI2sAudioInit(&pins)) {
    DEBUG_AUDIO_LOG("I2S init failed (check pins)");
    return false;
  }
  walleI2sAudioSetVolume(80);
  DEBUG_AUDIO_LOG("SD+I2S audio ready sd=%d", s_sdOk ? 1 : 0);
  return walleI2sAudioIsReady();
}

void audioPlayTrack(uint8_t trackId, uint8_t priority) {
  if (!walleI2sAudioIsReady()) {
    return;
  }
  if (priority < g_audioPriority && audioIsBusy()) {
    DEBUG_AUDIO_LOG("Skip track %u (priority %u < %u)", trackId, priority, g_audioPriority);
    return;
  }
  char path[64];
  if (!trackPath(trackId, path, sizeof(path))) {
    return;
  }
  if (!mountStorage() || !SD.exists(path)) {
    DEBUG_AUDIO_LOG("Missing %s", path);
    return;
  }
  g_audioPriority = priority;
  s_playPriority = priority;
  walleI2sAudioPlayFile(path);
  DEBUG_AUDIO_LOG("Play track %u -> %s", trackId, path);
}

void audioPlayStartup(void) {
  audioPlayTrack(TRACK_STARTUP, PRIO_NORMAL);
}
void audioPlayHello(void) {
  audioPlayTrack(TRACK_HELLO, PRIO_NORMAL);
}
void audioPlayAck(void) {
  audioPlayTrack(TRACK_ACK, PRIO_ACK);
}
void audioPlayError(void) {
  audioPlayTrack(TRACK_ERROR, PRIO_ERROR);
}
void audioPlayCurious(void) {
  audioPlayTrack(TRACK_CURIOUS, PRIO_NORMAL);
}
void audioPlaySleep(void) {
  audioPlayTrack(TRACK_SLEEP, PRIO_NORMAL);
}
void audioPlayWake(void) {
  audioPlayTrack(TRACK_WAKE, PRIO_NORMAL);
}
void audioPlayDockGuide(void) {
  audioPlayTrack(TRACK_DOCK_GUIDE, PRIO_NORMAL);
}
void audioPlayStop(void) {
  audioPlayTrack(TRACK_STOP, PRIO_VOICE_CMD);
}

void audioStop(void) {
  walleI2sAudioStop();
  g_audioPriority = 0;
  s_playPriority = 0;
  DEBUG_AUDIO_LOG("Stop");
}

void audioSetVolume(uint8_t vol) {
  if (vol > 100) {
    vol = 100;
  }
  walleI2sAudioSetVolume(vol);
  DEBUG_AUDIO_LOG("Volume %u", vol);
}

bool audioIsReady(void) {
  return walleI2sAudioIsReady();
}

bool audioIsBusy(void) {
  return walleI2sAudioIsPlaying();
}

void audioPlayerTick(void) {
  walleI2sAudioTick();
  if (!walleI2sAudioIsPlaying() && s_playPriority != 0) {
    g_audioPriority = 0;
    s_playPriority = 0;
  }
}
