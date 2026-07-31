/**
 * Host-side check for O-4 connection fact helpers (no Arduino required).
 */
#include "../awareness/eve_awareness_connection.h"

#include <cstdio>
#include <cstdlib>

int main(void) {
  if (!eveAwarenessPeerLabelIsLinked("wall_e")) {
    std::fprintf(stderr, "FAIL peer linked\n");
    return 1;
  }
  if (eveAwarenessPeerLabelIsLinked("none")) {
    std::fprintf(stderr, "FAIL peer none\n");
    return 1;
  }
  if (!eveAwarenessWallELinkedFromSession(0x100u, true)) {
    std::fprintf(stderr, "FAIL wallE linked\n");
    return 1;
  }
  if (eveAwarenessWallELinkedFromSession(0u, true)) {
    std::fprintf(stderr, "FAIL wallE no session\n");
    return 1;
  }
  if (eveAwarenessWallELinkedFromSession(0x100u, false)) {
    std::fprintf(stderr, "FAIL wallE no peer\n");
    return 1;
  }

  std::printf("awareness_o4_connection_test: OK\n");
  return 0;
}
