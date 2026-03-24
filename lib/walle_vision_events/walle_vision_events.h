/**
 * Vision → master high-level events (distinct from VisionPacket_t tracking feed).
 * vision_esp may send these alongside or instead of raw bbox packets.
 */
#pragma once
#include <stdint.h>

#define WALLE_VISION_EVENT_MAGIC 0x57455654u /* "WEVT" */
#define WALLE_VISION_EVENT_VERSION 1u

typedef enum : uint8_t {
  WALLE_VISION_EVT_NONE = 0,
  WALLE_VISION_EVT_HUMAN = 1,
  WALLE_VISION_EVT_MARKER = 2,
  WALLE_VISION_EVT_OBSTACLE = 3,
} walle_vision_event_type_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t version;
  uint8_t event_type;
  int16_t norm_x;
  int16_t norm_y;
  uint16_t confidence;
  uint32_t timestamp_ms;
} WalleVisionEventPacket_t;
