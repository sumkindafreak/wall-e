#pragma once

/*
 * EVE microphone input.
 *
 * Common digital mic modules such as INMP441, SPH0645, and ICS-43434 are I2S
 * microphones, not I2C audio devices. Leave MIC_ENABLED at 0 until you provide
 * actual BCLK / WS / DATA pins for your EVE board.
 */

#include <Arduino.h>

#ifndef MIC_ENABLED
#define MIC_ENABLED 0
#endif

#define MIC_TYPE_I2S 1
#define MIC_TYPE_I2C 2

#ifndef MIC_TYPE
#define MIC_TYPE MIC_TYPE_I2S
#endif

#ifndef MIC_SAMPLE_RATE
#define MIC_SAMPLE_RATE 16000
#endif

#ifndef MIC_I2S_BCLK_PIN
#define MIC_I2S_BCLK_PIN (-1)
#endif
#ifndef MIC_I2S_WS_PIN
#define MIC_I2S_WS_PIN (-1)
#endif
#ifndef MIC_I2S_DATA_PIN
#define MIC_I2S_DATA_PIN (-1)
#endif

// Only use these if you later select a genuine I2C/SPI microphone/control chip.
#ifndef MIC_SPI_OR_I2C_ADDRESS
#define MIC_SPI_OR_I2C_ADDRESS 0x00
#endif

#ifndef SOUND_SPIKE_THRESHOLD
#define SOUND_SPIKE_THRESHOLD 1800.0f
#endif
#ifndef CLAP_THRESHOLD
#define CLAP_THRESHOLD 4200.0f
#endif
#ifndef QUIET_THRESHOLD
#define QUIET_THRESHOLD 180.0f
#endif
#ifndef MIC_REACTION_COOLDOWN_MS
#define MIC_REACTION_COOLDOWN_MS 2500u
#endif

enum EveMicEvent : uint8_t {
  EVE_MIC_EVENT_NONE = 0,
  EVE_MIC_EVENT_SOUND_SPIKE,
  EVE_MIC_EVENT_CLAP,
  EVE_MIC_EVENT_QUIET_ROOM,
  EVE_MIC_EVENT_LOUD_ENVIRONMENT,
  EVE_MIC_EVENT_REPEATED_PATTERN
};

struct EveMicSettings {
  bool reactionsEnabled;
  float spikeThreshold;
  float clapThreshold;
  float quietThreshold;
  uint32_t reactionCooldownMs;
};

void initMic(void);
void updateMic(void);

float getMicLevel(void);
float getMicAmbientAverage(void);
bool detectSoundSpike(void);
bool detectClap(void);
bool isRoomQuiet(void);
bool isMicConfigured(void);
bool isMicRunning(void);

EveMicEvent micConsumeEvent(void);
const char* micEventName(EveMicEvent event);
String getMicStatusJson(void);

EveMicSettings micGetSettings(void);
void micSetSettings(const EveMicSettings& settings);
void micSetReactionsEnabled(bool enabled);
bool micReactionsEnabled(void);

