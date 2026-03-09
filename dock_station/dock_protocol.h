/*******************************************************************************
 * dock_protocol.h
 * ESP-NOW beacon packet structure and state enums for Smart Charging Crate
 ******************************************************************************/

#ifndef DOCK_PROTOCOL_H
#define DOCK_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/*=============================================================================
 * DOCK STATES (mirrors dock_state.h for protocol)
 *===========================================================================*/

enum DockState {
  STATE_BOOT = 0,
  STATE_NOT_DOCKED,
  STATE_DOCKED_IDLE,
  STATE_CHARGING,
  STATE_CHARGED,
  STATE_FAULT
};

/*=============================================================================
 * ESP-NOW BEACON PACKET
 * Sent at 10 Hz for WALL-E to home in via RSSI.
 *===========================================================================*/

#define DOCK_BEACON_MAGIC  0x444F434B  /* "DOCK" in ASCII */

#pragma pack(push, 1)

typedef struct {
  uint32_t magic;           /* DOCK_BEACON_MAGIC */
  uint32_t dock_id;         /* Unique dock identifier */
  uint32_t uptime_ms;       /* Millis since boot */
  uint8_t  state;           /* DockState enum */
  uint8_t  beam_present;    /* 1 = robot present (beam broken) */
  uint8_t  mouth_blocked;   /* 1 = obstacle detected */
  uint8_t  charge_enabled;  /* 1 = MOSFET on */
  int16_t  current_a_x100;  /* Current in A * 100 (e.g. 125 = 1.25A) */
} DockBeaconPacket_t;

#pragma pack(pop)

#define DOCK_BEACON_SIZE   sizeof(DockBeaconPacket_t)

/*=============================================================================
 * DOCK COMMAND PACKET (WALL-E -> dock via ESP-NOW)
 *===========================================================================*/

#define DOCK_CMD_MAGIC  0x434D444B  /* "CMDK" */

typedef struct {
  uint32_t magic;    /* DOCK_CMD_MAGIC */
  uint32_t dock_id;  /* Target dock (0 = any) */
  uint8_t  cmd;      /* DockCommand enum */
  uint8_t  pad[3];
} DockCommandPacket_t;

#define DOCK_CMD_SIZE  sizeof(DockCommandPacket_t)

enum DockCommand {
  DOCK_CMD_NONE        = 0,
  DOCK_CMD_FORCE_OFF   = 1,   /* Force charging off, enter fault */
  DOCK_CMD_RESET       = 2,   /* Clear fault, return to state machine */
  DOCK_CMD_WIFI_CONFIG = 3,   /* WiFi credentials from WALL-E */
  DOCK_CMD_TIME        = 4    /* Unix timestamp from WALL-E */
};

/* Time packet (cmd = DOCK_CMD_TIME) — sent by WALL-E */
typedef struct {
  uint32_t magic;    /* DOCK_CMD_MAGIC */
  uint32_t dock_id;  /* Target dock (0 = any) */
  uint8_t  cmd;      /* DOCK_CMD_TIME */
  uint8_t  pad[3];
  uint32_t unix_time; /* Unix timestamp (seconds since 1970) */
} DockTimePacket_t;

#define DOCK_TIME_SIZE  sizeof(DockTimePacket_t)

/* WiFi config packet (cmd = DOCK_CMD_WIFI_CONFIG) — sent by WALL-E */
typedef struct {
  uint32_t magic;    /* DOCK_CMD_MAGIC */
  uint32_t dock_id;  /* Target dock (0 = any) */
  uint8_t  cmd;      /* DOCK_CMD_WIFI_CONFIG */
  uint8_t  pad[3];
  char     ssid[33]; /* SSID, null-terminated, max 32 chars */
  char     pass[65]; /* Password, null-terminated, max 64 chars */
} DockWifiConfigPacket_t;

#define DOCK_WIFI_CONFIG_SIZE  sizeof(DockWifiConfigPacket_t)

#endif /* DOCK_PROTOCOL_H */
