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

#define DOCK_CMD_MAGIC  0x434D444B  /* "CMDK" */

typedef struct __attribute__((packed)) {
  uint32_t magic;    /* DOCK_CMD_MAGIC */
  uint32_t dock_id;  /* Target dock (0 = any) */
  uint8_t  cmd;
  uint8_t  pad[3];
} DockCommandPacket_t;

#define DOCK_CMD_NONE            0
#define DOCK_CMD_FORCE_OFF       1
#define DOCK_CMD_RESET           2
#define DOCK_CMD_WIFI_CONFIG     3
#define DOCK_CMD_TIME            4
#define DOCK_CMD_REQUEST_CHARGE  5
#define DOCK_CMD_APPROACH_STAGE  6
#define DOCK_CMD_DOCKING_ARM     7
#define DOCK_CMD_DOCKING_DISARM  8

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

#pragma pack(push, 1)
typedef struct {
  uint32_t magic;
  uint32_t dock_id;
  uint8_t  cmd;
  uint8_t  pad[3];
  uint32_t unix_time;
} DockTimePacket_t;

typedef struct {
  uint32_t magic;
  uint32_t dock_id;
  uint8_t  cmd;
  uint8_t  pad[3];
  char     ssid[33];
  char     pass[65];
} DockWifiConfigPacket_t;
#pragma pack(pop)

#define DOCK_CMD_SIZE              sizeof(DockCommandPacket_t)
#define DOCK_APPROACH_STAGE_SIZE   sizeof(DockApproachStagePacket_t)
#define DOCK_TIME_SIZE             sizeof(DockTimePacket_t)
#define DOCK_WIFI_CONFIG_SIZE      sizeof(DockWifiConfigPacket_t)

#endif /* DOCK_PROTOCOL_H */
