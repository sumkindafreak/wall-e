#!/usr/bin/env sh
# Verify canonical wire-protocol headers exist (run from repo root).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CANON="$ROOT/firmware_common/include"
for f in node_health_protocol.h walle_link_packet.h audio_protocol.h; do
  if [ ! -f "$CANON/$f" ]; then
    echo "Missing canonical protocol header: $CANON/$f"
    exit 1
  fi
done
echo "OK: firmware_common protocol headers present."
exit 0
