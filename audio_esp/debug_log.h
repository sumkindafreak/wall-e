/**
 * debug_log.h — Debug output macros
 */
#ifndef AUDIO_ESP_DEBUG_LOG_H
#define AUDIO_ESP_DEBUG_LOG_H

#include "config.h"
#include <Arduino.h>

#define DEBUG_LOG(fmt, ...) \
  do { if (DEBUG_ENABLE) Serial.printf("[AUDIO_ESP] " fmt "\n", ##__VA_ARGS__); } while(0)

#define DEBUG_AUDIO_LOG(fmt, ...) \
  do { if (DEBUG_ENABLE && DEBUG_AUDIO) Serial.printf("[AUDIO] " fmt "\n", ##__VA_ARGS__); } while(0)

#define DEBUG_COMMS_LOG(fmt, ...) \
  do { if (DEBUG_ENABLE && DEBUG_COMMS) Serial.printf("[COMMS] " fmt "\n", ##__VA_ARGS__); } while(0)

#define DEBUG_MIC_LOG(fmt, ...) \
  do { if (DEBUG_ENABLE && DEBUG_MIC) Serial.printf("[MIC] " fmt "\n", ##__VA_ARGS__); } while(0)

#define DEBUG_IR_LOG(fmt, ...) \
  do { if (DEBUG_ENABLE && DEBUG_IR) Serial.printf("[IR] " fmt "\n", ##__VA_ARGS__); } while(0)

#endif
