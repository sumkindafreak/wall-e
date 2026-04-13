/**
 * voicebox_protocol.h — Shared WALL-E / EVE voice personality layer (ESP-NOW + telemetry).
 * Magic packets are versioned; reject unknown versions in firmware.
 */
#ifndef WALL_E_VOICEBOX_PROTOCOL_H
#define WALL_E_VOICEBOX_PROTOCOL_H

#include <stdint.h>

#define WALLE_VOICEBOX_PROTO_VERSION 1u

/** Base -> Audio: set operating mode (packed, ESP-NOW). Magic "VBXC" */
#define WALLE_VOICEBOX_CMD_MAGIC 0x56425843u

typedef enum {
  VOICEBOX_SOLO_WALLE = 0,
  VOICEBOX_SHARED_WALLE_EVE = 1,
} walle_voicebox_mode_t;

/* Pair-request IDs live in menu_protocol.h (walle_ui_pair_request_t). */

/**
 * Command from WALL-E base to Audio ESP: which voicebox personality is active.
 * When eve_audio_ok==0, Audio ESP stays responsive but should not expect EVE pairing.
 */
typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t version;
  uint8_t mode;           /* walle_voicebox_mode_t */
  uint8_t eve_audio_ok;   /* 1 if base can command EVE sound/expression */
  uint8_t bond_strength; /* 0–255 scaled hint for dual-line frequency */
} WalleVoiceboxCmdPacket_t;

#endif
