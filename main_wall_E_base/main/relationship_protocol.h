/**
 * relationship_protocol.h — WALL-E <-> EVE bond model (persisted on base; JSON in memory logs).
 */
#ifndef WALL_E_RELATIONSHIP_PROTOCOL_H
#define WALL_E_RELATIONSHIP_PROTOCOL_H

#include <stdint.h>

#define WALLE_REL_PREFS_NS "walle_rel"

typedef enum {
  WALLE_LAST_IX_NONE = 0,
  WALLE_LAST_IX_GREET = 1,
  WALLE_LAST_IX_DOCK = 2,
  WALLE_LAST_IX_BUTTON = 3,
  WALLE_LAST_IX_VOICE = 4,
  WALLE_LAST_IX_VISION = 5,
} walle_last_interaction_t;

typedef struct {
  uint32_t last_seen_together_ms_epoch; /* millis() snapshot when both online */
  uint32_t attachment_count;
  uint32_t cooperative_moments;
  uint32_t shared_dock_events;
  uint8_t comfort_level;   /* 0–100 */
  uint8_t trust_level;
  uint8_t curiosity_level;
  uint8_t bond_strength;   /* derived 0–100 */
  uint8_t last_interaction_type;
} WalleBondState;

#endif
