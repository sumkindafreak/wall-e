#!/usr/bin/env sh
# Verify canonical wire-protocol headers exist and that copies stay in sync (run from repo root).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CANON="$ROOT/firmware_common/include"
for f in node_health_protocol.h walle_link_packet.h audio_protocol.h eve_link_protocol.h; do
  if [ ! -f "$CANON/$f" ]; then
    echo "Missing canonical protocol header: $CANON/$f"
    exit 1
  fi
done

# The EVE dock keeps its own copy of the EVE<->peer message IDs it uses. Every ID it defines must
# have the same value in the canonical eve_link_protocol.h (EVE and WALL-E include that one).
EVE_CANON="$CANON/eve_link_protocol.h"
DOCK="$ROOT/eve_dock_esp32s3/eve_protocol.h"
if [ -f "$DOCK" ]; then
  bad=0
  for line in $(grep -oE 'MSG_[A-Z0-9_]+ *= *0x[0-9A-Fa-f]+' "$DOCK" | tr -d ' '); do
    name="${line%%=*}"
    val="$(echo "${line#*=}" | tr 'A-F' 'a-f')"
    canon="$(grep -oE "$name *= *0x[0-9A-Fa-f]+" "$EVE_CANON" | head -n1 | sed 's/.*= *//' | tr 'A-F' 'a-f')"
    if [ -z "$canon" ]; then
      echo "Dock ID $name ($val) missing from $EVE_CANON"
      bad=1
    elif [ "$canon" != "$val" ]; then
      echo "Dock ID $name is $val but canonical value is $canon"
      bad=1
    fi
  done
  for c in EVE_SOF0 EVE_SOF1 EVE_FRAME_VER EVE_MAX_PAYLOAD; do
    d="$(grep -oE "$c *= *[0-9xA-Fa-f]+" "$DOCK" | head -n1 | sed 's/.*= *//')"
    k="$(grep -oE "$c *= *[0-9xA-Fa-f]+" "$EVE_CANON" | head -n1 | sed 's/.*= *//')"
    if [ "$d" != "$k" ]; then
      echo "Framing constant $c differs: dock=$d canonical=$k"
      bad=1
    fi
  done
  if [ "$bad" -ne 0 ]; then
    exit 1
  fi
fi

# EVE's sketch must use the canonical header, not a private copy.
if ! grep -q 'firmware_common/include/eve_link_protocol.h' "$ROOT/eve/arduino_ide/EVE_Companion/eve_protocol.h"; then
  echo "eve/arduino_ide/EVE_Companion/eve_protocol.h no longer includes the canonical eve_link_protocol.h"
  exit 1
fi

echo "OK: firmware_common protocol headers present; EVE/dock message IDs in sync."
exit 0
