#!/usr/bin/env bash
# Compile all PlatformIO firmware modules (requires PlatformIO + network on first run).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="${HOME}/.local/bin:${PATH}"
"$ROOT/scripts/verify_protocol_headers.sh"
"$ROOT/scripts/gen_showduino_version.sh"

builds=(
  "main_wall_E_base:wall_e_brain_s3:Wall-E Base"
  "eve:eve_s3:EVE"
  "audio_esp:esp32-s3-devkitc-1:Audio ESP"
  "dock_station:dock_esp32:Dock"
  "wall_e_master_controller:cyd_esp32_2432s028:Master Controller"
  "vision_node:vision_s3:Vision Node"
  "ghostbusters_slime_blower:esp32-s3-devkitc-1:Ghostbusters"
)

for entry in "${builds[@]}"; do
  IFS=':' read -r path env name <<< "$entry"
  echo ""
  echo "[$name] pio run -e $env ($path)"
  (cd "$ROOT/$path" && pio run -e "$env")
done
echo ""
echo "All firmware targets built successfully."
