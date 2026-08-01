/**
 * EVE Phase O-4 — connection facts (present tense only; no history/latch fields).
 */
#pragma once

#include <stdint.h>
#include <string.h>

/** True when peer label is a live partner (not empty / "none"). */
static inline bool eveAwarenessPeerLabelIsLinked(const char* peerLabel) {
  return peerLabel && peerLabel[0] != '\0' && strcmp(peerLabel, "none") != 0;
}

static inline bool eveAwarenessWallELinkedFromSession(uint32_t sessionId, bool peerLinked) {
  return sessionId != 0u && peerLinked;
}
