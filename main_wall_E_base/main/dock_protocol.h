/*******************************************************************************
 * dock_protocol.h
 * Shared ESP-NOW beacon structure for dock ↔ WALL-E base
 * Mirrors dock_station/dock_protocol.h — keep in sync
 ******************************************************************************/

#ifndef DOCK_PROTOCOL_H
#define DOCK_PROTOCOL_H

#include <stdint.h>

#define DOCK_BEACON_MAGIC  0x444F434B  /* "DOCK" in ASCII */

#define DOCK_IR_ALIGN_LOST    0u
#define DOCK_IR_ALIGN_LEFT    1u
#define DOCK_IR_ALIGN_RIGHT   2u
#define DOCK_IR_ALIGN_CENTER  3u
#define DOCK_IR_ALIGN_PAUSE   4u

#pragma pack(push, 1)
typedef struct {
  uint32_t magic;
  uint32_t dock_id;
  uint32_t uptime_ms;
  uint8_t  state;
  uint8_t  beam_present;
  uint8_t  mouth_blocked;
  uint8_t  charge_enabled;
  uint8_t  callout_active;  /* 1 = dock calling WALL-E */
  int16_t  current_a_x100;
  uint8_t  ir_align_hint;
} DockBeaconPacket_t;
#pragma pack(pop)

#define DOCK_BEACON_SIZE   sizeof(DockBeaconPacket_t)

/* DOCK_CMD_APPROACH_STAGE: WALL-E reports approach stage for arrow staging */
#define DOCK_CMD_APPROACH_STAGE  6

typedef enum {
  APPROACH_FAR   = 0,  /* >1m: arrows off / ready pulse */
  APPROACH_1M    = 1,  /* 200mm–1m: arrows activate */
  APPROACH_20CM  = 2,  /* 60–200mm: precision */
  APPROACH_DOCKED = 3
} DockApproachStage_t;

#pragma pack(push, 1)
typedef struct {
  uint32_t magic;
  uint32_t dock_id;
  uint8_t  cmd;
  uint8_t  stage;
  uint8_t  pad[2];
} DockApproachStagePacket_t;
#pragma pack(pop)

#endif /* DOCK_PROTOCOL_H */
