/**
 * EVE Phase O-4 — connection facts (present tense only; no history/latch fields).
 */
#pragma once

#include <stdint.h>

/** True when peer label is a live partner (not empty / "none"). */
static inline bool eveAwarenessPeerLabelIsLinked(const char* peerLabel) {
  if (!peerLabel || peerLabel[0] == '\0') {
    return false;
  }
  return !(peerLabel[0] == 'n' && peerLabel[1] == 'o' && peerLabel[2] == 'n' &&
           peerLabel[3] == 'e' && peerLabel[4] == '\0');
}

static inline bool eveAwarenessWallELinkedFromSession(uint32_t sessionId, bool peerLinked) {
  return sessionId != 0u && peerLinked;
}
