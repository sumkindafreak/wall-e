/**
 * WALL-E / EVE — SD-card WAV playback over I2S.
 */
#include "walle_i2s_wav_player.h"

#include <FS.h>
#include <SD.h>
#include <driver/i2s.h>
#include <string.h>

#ifndef WALLE_I2S_QUEUE_MAX
#define WALLE_I2S_QUEUE_MAX 8
#endif
#ifndef WALLE_I2S_CHUNK_BYTES
#define WALLE_I2S_CHUNK_BYTES 1024
#endif

struct WavFmt {
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint32_t dataOffset;
  uint32_t dataSize;
};

static struct {
  bool driverOk;
  WalleI2sPinConfig pins;
  i2s_port_t port;
  uint8_t volume;
  bool playing;
  bool paused;
  File file;
  WavFmt fmt;
  uint32_t dataRemaining;
  char currentPath[128];
  char queue[WALLE_I2S_QUEUE_MAX][128];
  uint8_t qCount;
  WalleAudioEventFn callback;
  WalleAudioStatus status;
} s_audio;

static void emitEvent(WalleAudioEvent evt) {
  if (s_audio.callback) {
    s_audio.callback(evt, s_audio.currentPath[0] ? s_audio.currentPath : nullptr);
  }
}

static i2s_port_t portFromIndex(uint8_t idx) {
  return (idx == 0) ? I2S_NUM_0 : I2S_NUM_1;
}

static bool pinsValid(const WalleI2sPinConfig* p) {
  return p && p->bclkPin >= 0 && p->lrckPin >= 0 && p->doutPin >= 0;
}

static bool parseWavHeader(File& f, WavFmt* out) {
  if (!out || !f) {
    return false;
  }
  memset(out, 0, sizeof(*out));
  f.seek(0);
  char riff[4];
  if (f.read((uint8_t*)riff, 4) != 4 || memcmp(riff, "RIFF", 4) != 0) {
    return false;
  }
  f.seek(8);
  char wave[4];
  if (f.read((uint8_t*)wave, 4) != 4 || memcmp(wave, "WAVE", 4) != 0) {
    return false;
  }

  uint32_t pos = 12;
  const uint32_t fileSize = f.size();
  while (pos + 8 <= fileSize) {
    f.seek(pos);
    char id[4];
    uint32_t chunkSize = 0;
    if (f.read((uint8_t*)id, 4) != 4) {
      return false;
    }
    if (f.read((uint8_t*)&chunkSize, 4) != 4) {
      return false;
    }
    pos += 8;
    if (memcmp(id, "fmt ", 4) == 0 && chunkSize >= 16) {
      f.seek(pos);
      f.read((uint8_t*)&out->audioFormat, 2);
      f.read((uint8_t*)&out->numChannels, 2);
      f.read((uint8_t*)&out->sampleRate, 4);
      f.seek(pos + 14);
      f.read((uint8_t*)&out->bitsPerSample, 2);
    } else if (memcmp(id, "data", 4) == 0) {
      out->dataOffset = pos;
      out->dataSize = chunkSize;
      break;
    }
    pos += chunkSize + (chunkSize & 1u);
  }

  return (out->audioFormat == 1 && out->bitsPerSample == 16 && out->dataSize > 0 && out->numChannels <= 2);
}

static bool installI2sDriver(uint32_t sampleRate) {
  if (!s_audio.driverOk) {
    return false;
  }
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = (int)sampleRate;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
#if defined(I2S_COMM_FORMAT_STAND_I2S)
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
#else
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
#endif
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;

  i2s_driver_uninstall(s_audio.port);
  if (i2s_driver_install(s_audio.port, &cfg, 0, nullptr) != ESP_OK) {
    return false;
  }

  i2s_pin_config_t pinCfg = {};
  pinCfg.bck_io_num = s_audio.pins.bclkPin;
  pinCfg.ws_io_num = s_audio.pins.lrckPin;
  pinCfg.data_out_num = s_audio.pins.doutPin;
  pinCfg.data_in_num = I2S_PIN_NO_CHANGE;
  return i2s_set_pin(s_audio.port, &pinCfg) == ESP_OK;
}

static void closeCurrentFile(void) {
  if (s_audio.file) {
    s_audio.file.close();
  }
  s_audio.dataRemaining = 0;
}

static bool startFile(const char* path) {
  closeCurrentFile();
  strncpy(s_audio.currentPath, path ? path : "", sizeof(s_audio.currentPath) - 1);
  s_audio.currentPath[sizeof(s_audio.currentPath) - 1] = '\0';

  s_audio.file = SD.open(path, FILE_READ);
  if (!s_audio.file) {
    s_audio.status = WALLE_AUDIO_ERROR;
    emitEvent(WALLE_AUDIO_EVT_ERROR);
    return false;
  }
  if (!parseWavHeader(s_audio.file, &s_audio.fmt)) {
    closeCurrentFile();
    s_audio.status = WALLE_AUDIO_ERROR;
    emitEvent(WALLE_AUDIO_EVT_ERROR);
    return false;
  }
  if (!installI2sDriver(s_audio.fmt.sampleRate)) {
    closeCurrentFile();
    s_audio.status = WALLE_AUDIO_ERROR;
    emitEvent(WALLE_AUDIO_EVT_ERROR);
    return false;
  }
  s_audio.file.seek(s_audio.fmt.dataOffset);
  s_audio.dataRemaining = s_audio.fmt.dataSize;
  s_audio.playing = true;
  s_audio.paused = false;
  s_audio.status = WALLE_AUDIO_PLAYING;
  emitEvent(WALLE_AUDIO_EVT_STARTED);
  return true;
}

static bool popQueue(char* out, size_t outLen) {
  if (s_audio.qCount == 0 || !out || outLen == 0) {
    return false;
  }
  strncpy(out, s_audio.queue[0], outLen - 1);
  out[outLen - 1] = '\0';
  for (uint8_t i = 1; i < s_audio.qCount; ++i) {
    strncpy(s_audio.queue[i - 1], s_audio.queue[i], sizeof(s_audio.queue[i - 1]) - 1);
  }
  --s_audio.qCount;
  return true;
}

static void finishCurrentTrack(void) {
  closeCurrentFile();
  s_audio.playing = false;
  s_audio.paused = false;
  emitEvent(WALLE_AUDIO_EVT_COMPLETED);

  char next[128];
  if (popQueue(next, sizeof(next))) {
    startFile(next);
    return;
  }
  s_audio.status = WALLE_AUDIO_IDLE;
  s_audio.currentPath[0] = '\0';
  emitEvent(WALLE_AUDIO_EVT_QUEUE_EMPTY);
}

static void applyVolume(int16_t* samples, size_t count) {
  if (s_audio.volume >= 100) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    samples[i] = (int16_t)((int32_t)samples[i] * (int32_t)s_audio.volume / 100);
  }
}

bool walleI2sAudioInit(const WalleI2sPinConfig* pins) {
  memset(&s_audio, 0, sizeof(s_audio));
  s_audio.volume = 80;
  s_audio.status = WALLE_AUDIO_IDLE;
  if (!pinsValid(pins)) {
    s_audio.driverOk = false;
    return false;
  }
  s_audio.pins = *pins;
  s_audio.port = portFromIndex(pins->portIndex);
  s_audio.driverOk = true;
  return true;
}

void walleI2sAudioSetEventCallback(WalleAudioEventFn fn) {
  s_audio.callback = fn;
}

bool walleI2sAudioIsReady(void) {
  return s_audio.driverOk;
}

WalleAudioStatus walleI2sAudioGetStatus(void) {
  return s_audio.status;
}

bool walleI2sAudioIsPlaying(void) {
  return s_audio.playing && !s_audio.paused;
}

void walleI2sAudioSetVolume(uint8_t pct) {
  s_audio.volume = pct > 100 ? 100 : pct;
}

uint8_t walleI2sAudioGetVolume(void) {
  return s_audio.volume;
}

void walleI2sAudioClearQueue(void) {
  s_audio.qCount = 0;
}

uint8_t walleI2sAudioQueueDepth(void) {
  return s_audio.qCount;
}

bool walleI2sAudioQueueFile(const char* path) {
  if (!path || path[0] == '\0' || s_audio.qCount >= WALLE_I2S_QUEUE_MAX) {
    return false;
  }
  strncpy(s_audio.queue[s_audio.qCount], path, sizeof(s_audio.queue[0]) - 1);
  s_audio.queue[s_audio.qCount][sizeof(s_audio.queue[0]) - 1] = '\0';
  ++s_audio.qCount;
  return true;
}

bool walleI2sAudioPlayFile(const char* path) {
  if (!s_audio.driverOk || !path || path[0] == '\0') {
    return false;
  }
  walleI2sAudioStop();
  return startFile(path);
}

void walleI2sAudioStop(void) {
  closeCurrentFile();
  s_audio.playing = false;
  s_audio.paused = false;
  s_audio.status = WALLE_AUDIO_IDLE;
  walleI2sAudioClearQueue();
  emitEvent(WALLE_AUDIO_EVT_STOPPED);
  s_audio.currentPath[0] = '\0';
}

void walleI2sAudioPause(void) {
  if (!s_audio.playing || s_audio.paused) {
    return;
  }
  s_audio.paused = true;
  s_audio.status = WALLE_AUDIO_PAUSED;
  emitEvent(WALLE_AUDIO_EVT_PAUSED);
}

void walleI2sAudioResume(void) {
  if (!s_audio.playing || !s_audio.paused) {
    return;
  }
  s_audio.paused = false;
  s_audio.status = WALLE_AUDIO_PLAYING;
  emitEvent(WALLE_AUDIO_EVT_RESUMED);
}

void walleI2sAudioTick(void) {
  if (!s_audio.driverOk || !s_audio.playing || s_audio.paused || !s_audio.file) {
    return;
  }

  uint8_t buf[WALLE_I2S_CHUNK_BYTES];
  size_t toRead = WALLE_I2S_CHUNK_BYTES;
  if (toRead > s_audio.dataRemaining) {
    toRead = s_audio.dataRemaining;
  }
  if (toRead == 0) {
    finishCurrentTrack();
    return;
  }

  size_t n = s_audio.file.read(buf, toRead);
  if (n == 0) {
    finishCurrentTrack();
    return;
  }
  s_audio.dataRemaining -= (uint32_t)n;

  size_t sampleCount = n / sizeof(int16_t);
  applyVolume((int16_t*)buf, sampleCount);

  size_t written = 0;
  i2s_write(s_audio.port, buf, n, &written, pdMS_TO_TICKS(20));
  if (n > 0 && written == 0) {
    /* DMA back-pressure — retry next tick */
    s_audio.file.seek(s_audio.file.position() - (int32_t)n);
    s_audio.dataRemaining += (uint32_t)n;
  }

  if (s_audio.dataRemaining == 0) {
    finishCurrentTrack();
  }
}
