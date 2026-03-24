/**
 * WALL-E node health — periodic ESP-NOW status (shared: base, audio, dock, CYD).
 * Copy of ../wall_e_audio/node_health_protocol.h — keep in sync.
 */
#ifndef WALLE_NODE_HEALTH_PROTOCOL_H
#define WALLE_NODE_HEALTH_PROTOCOL_H

#include <stdint.h>

/** "WNHT" */
#define WALLE_NODE_HEALTH_MAGIC 0x574E4854u
#define WALLE_NODE_HEALTH_VERSION 1u

typedef enum {
  WALLE_NODE_BASE = 0,
  WALLE_NODE_MASTER = 1,
  WALLE_NODE_AUDIO = 2,
  WALLE_NODE_DOCK = 3,
  WALLE_NODE_VISION = 4,
  WALLE_NODE_COUNT = 5
} walle_node_id_t;

typedef enum {
  WALLE_ROLE_UNSPEC = 0,
  WALLE_ROLE_OPERATOR = 1,
  WALLE_ROLE_SENSOR = 2,
  WALLE_ROLE_ACTOR = 3,
} walle_node_role_t;

/** battery_pct: 0–100, or unknown */
#define WALLE_NODE_HEALTH_UNKNOWN_BAT 255u

/** temp_c: -128 = unknown / not available */
#define WALLE_NODE_HEALTH_UNKNOWN_TEMP (-128)

#define WALLE_NODE_FLAG_DOCKED   (1u << 0)
#define WALLE_NODE_FLAG_CHARGING (1u << 1)
#define WALLE_NODE_FLAG_FAULT    (1u << 2)

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t  version;
  uint8_t  node_id;
  uint8_t  role;
  uint8_t  battery_pct;
  int8_t   temp_c;
  uint32_t uptime_ms;
  uint16_t last_error;
  uint16_t flags;
} WalleNodeHealthPacket_t;

#endif
