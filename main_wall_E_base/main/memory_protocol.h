/**
 * memory_protocol.h — On-robot event log layout (LittleFS when available).
 */
#ifndef WALL_E_MEMORY_PROTOCOL_H
#define WALL_E_MEMORY_PROTOCOL_H

#define WALLE_MEMORY_DIR "/memory"
#define WALLE_MEMORY_EVENTS "events.jsonl"
#define WALLE_MEMORY_BOND "eve_bond_state.json"
#define WALLE_MEMORY_DOCK "dock_history.jsonl"
#define WALLE_MEMORY_TELEM "telemetry_log.jsonl"
#define WALLE_MEMORY_MISSION "mission_log.jsonl"
#define WALLE_MEMORY_VOICEBOX "voicebox_state.json"

typedef enum {
  WALLE_MEM_EV_BOOT = 1,
  WALLE_MEM_EV_NODE_ONLINE = 2,
  WALLE_MEM_EV_NODE_OFFLINE = 3,
  WALLE_MEM_EV_EVE_ATTACH = 4,
  WALLE_MEM_EV_EVE_DETACH = 5,
  WALLE_MEM_EV_VOICEBOX_MODE = 6,
  WALLE_MEM_EV_BUTTON = 7,
  WALLE_MEM_EV_MENU = 8,
  WALLE_MEM_EV_AUDIO = 9,
  WALLE_MEM_EV_DOCK = 10,
  WALLE_MEM_EV_BATTERY = 11,
  WALLE_MEM_EV_SAFETY = 12,
} walle_memory_event_type_t;

#endif
