/***************************************************************
  Minimal config for IAN audio engine (wall-e-heart)
  Only audio-related defines
***************************************************************/

#ifndef IAN_CONFIG_H
#define IAN_CONFIG_H

#include <stdint.h>

#define I2S_SAMPLE_RATE         44100
#define I2S_BITS_PER_SAMPLE     16
#define I2S_CHANNELS            2
#define AUDIO_BUFFER_SIZE       4096

enum AudioState : uint8_t {
  AUDIO_STOPPED = 0,
  AUDIO_PLAYING = 1,
  AUDIO_PAUSED = 2,
  AUDIO_ERROR = 3
};

#endif
