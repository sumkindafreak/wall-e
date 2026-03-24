/**
 * Cross-node audio personality events (ESP-NOW payload; expand as needed).
 * Playback mapping lives on audio_esp — keep structs packed and versioned.
 */
#pragma once
#include <stdint.h>

#define WALLE_AUDIO_EVENT_MAGIC 0x57414556u /* "WAEV" */
#define WALLE_AUDIO_EVENT_VERSION 1u

typedef enum : uint8_t {
  WALLE_AUDIO_EVT_NONE = 0,
  WALLE_AUDIO_EVT_EMOTION_BASE = 16,
} walle_audio_event_type_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t version;
  uint8_t event_id;
  uint8_t priority;
  uint8_t pad;
} WalleAudioEventPacket_t;
