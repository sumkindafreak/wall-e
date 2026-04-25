/**
 * WALL-E node health — periodic ESP-NOW status (base, audio, dock, CYD).
 * v2 adds: heap, loop p95, RSSI, SD, last peer age.
 *
 * Canonical copy: include this path from all firmware (see firmware_common/).
 */
#ifndef WALLE_NODE_HEALTH_PROTOCOL_H
#define WALLE_NODE_HEALTH_PROTOCOL_H

#include <stdint.h>

#define WALLE_NODE_HEALTH_MAGIC 0x574E4854u
#define WALLE_NODE_HEALTH_VERSION 2u
#define WALLE_NODE_HEALTH_V1_SIZE 18u
#define WALLE_NODE_HEALTH_V2_SIZE 30u

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

#define WALLE_NODE_HEALTH_UNKNOWN_BAT 255u
#define WALLE_NODE_HEALTH_UNKNOWN_TEMP (-128)
#define WALLE_NODE_RSSI_UNKNOWN (-128)
#define WALLE_NODE_FLAG_DOCKED   (1u << 0)
#define WALLE_NODE_FLAG_CHARGING (1u << 1)
#define WALLE_NODE_FLAG_FAULT    (1u << 2)

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint8_t  version; /* 1 = first 18 B only, 2 = full V2 */
  uint8_t  node_id;
  uint8_t  role;
  uint8_t  battery_pct;
  int8_t   temp_c;
  uint32_t uptime_ms;
  uint16_t last_error;
  uint16_t flags;
  /* v2 (bytes 18–29) */
  uint32_t free_heap;          /* esp_get_free_heap or ESP.getFreeHeap() */
  uint16_t loop_p95_ms;        /* 0 = n/a (coarse) */
  int8_t   wifi_rssi;          /* WALLE_NODE_RSSI_UNKNOWN if n/a (e.g. AP with no client RSSI) */
  uint8_t  sd_ok;              /* 0/1, or 0 if no SD on node */
  uint32_t last_peer_rx_age_ms; /* age since last operator/controller packet; 0xFFFFFFFF = n/a */
  uint8_t  wire_pad;            /* explicit pad: natural packed layout = 29 B, wire / len checks = 30 */
} WalleNodeHealthPacket_t;

#if defined(__cplusplus) && __cplusplus >= 201103L
static_assert(sizeof(WalleNodeHealthPacket_t) == WALLE_NODE_HEALTH_V2_SIZE,
              "WalleNodeHealthPacket_t must match WALLE_NODE_HEALTH_V2_SIZE");
#endif

#endif
