/**
 * WALL-E audio brain ESP-NOW protocol (Base, master and audio ESP32-S3).
 * Copy of ../../wall_e_audio/audio_protocol.h — keep in sync.
 * Placed here so Arduino IDE finds it when compiling the Base source tree.
 */
#ifndef WALLE_AUDIO_PROTOCOL_H
#define WALLE_AUDIO_PROTOCOL_H

#include <stdint.h>

#define WALLE_AUDIO_MAGIC 0x574C4541u
#define WALLE_AUDIO_MIC_MAGIC 0x574D4943u

typedef enum {
  WALLE_AUDIO_EVT_NONE = 0,
  WALLE_AUDIO_EVT_BOOT = 1,
  WALLE_AUDIO_EVT_IDLE = 2,
  WALLE_AUDIO_EVT_CURIOUS = 3,
  WALLE_AUDIO_EVT_ESTOP = 4,
} walle_audio_event_t;

typedef enum {
  WALLE_AU_CMD_PLAY_EVENT = 0,
  WALLE_AU_CMD_PLAY_TRACK = 1,
  WALLE_AU_CMD_VOLUME = 2,
  WALLE_AU_CMD_STOP = 3,
} walle_au_cmd_t;

#define WALLE_AUDIO_PRIORITY_NORMAL 0u
#define WALLE_AUDIO_PRIORITY_WEB    64u
#define WALLE_AUDIO_PRIORITY_ESTOP  255u

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t cmd;
  uint8_t param;
  uint8_t reserved;
  uint8_t priority;
} WalleAudioCommandPacket_t;

typedef struct __attribute__((packed)) {
  uint8_t event;
  uint8_t volume;
  uint8_t priority;
} WalleAudioLegacyPacket_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint16_t ear_l;
  uint16_t ear_r;
  uint8_t voice_active;
  uint8_t reserved;
} WalleAudioMicTelemPacket_t;

#define WALLE_AUDIO_STATUS_MAGIC 0x57535441u

typedef enum {
  WALLE_AU_MIC_DIR_UNKNOWN = 0,
  WALLE_AU_MIC_DIR_LEFT = 1,
  WALLE_AU_MIC_DIR_RIGHT = 2,
  WALLE_AU_MIC_DIR_CENTER = 3,
} walle_au_mic_dir_t;

typedef enum {
  WALLE_AU_DOCK_IR_NONE = 0,
  WALLE_AU_DOCK_IR_LEFT = 1,
  WALLE_AU_DOCK_IR_RIGHT = 2,
  WALLE_AU_DOCK_IR_BOTH = 3,
  WALLE_AU_DOCK_IR_UNSTABLE = 4,
} walle_au_dock_ir_t;

typedef enum {
  WALLE_AU_VOICE_NONE = 0,
  WALLE_AU_VOICE_STOP = 1,
  WALLE_AU_VOICE_COME_HERE = 2,
  WALLE_AU_VOICE_GO_HOME = 3,
  WALLE_AU_VOICE_SLEEP = 4,
  WALLE_AU_VOICE_WAKE = 5,
} walle_au_voice_cmd_t;

typedef enum {
  WALLE_AU_MODE_BOOT = 0,
  WALLE_AU_MODE_IDLE = 1,
  WALLE_AU_MODE_LISTENING = 2,
  WALLE_AU_MODE_PLAYING = 3,
  WALLE_AU_MODE_DOCK_ASSIST = 4,
  WALLE_AU_MODE_VOICE_CMD = 5,
  WALLE_AU_MODE_FAULT = 6,
} walle_au_mode_t;

typedef enum {
  WALLE_AU_FAULT_NONE = 0,
  WALLE_AU_FAULT_DFPLAYER = 1,
} walle_au_fault_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t mic_dir;
  uint8_t dock_ir;
  uint8_t voice_cmd;
  uint8_t mode;
  uint8_t fault;
  uint8_t audio_busy;
  uint8_t reserved[2];
} WalleAudioStatusPacket_t;

#endif
