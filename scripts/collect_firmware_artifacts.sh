#!/usr/bin/env bash
# Copy PlatformIO firmware.bin outputs to dist/firmware/ with stable release names.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/dist/firmware}"
mkdir -p "$OUT"

copy_bin() {
  local rel_src="$1"
  local out_name="$2"
  local src="$ROOT/$rel_src"
  if [[ ! -f "$src" ]]; then
    echo "Missing firmware binary: $src (run build_all_firmware.sh first)" >&2
    exit 1
  fi
  cp "$src" "$OUT/$out_name"
  echo "  $out_name <= $rel_src"
}

echo "Collecting firmware artifacts -> $OUT"
copy_bin "main_wall_E_base/.pio/build/wall_e_brain_s3/firmware.bin" "wall_e_base.bin"
copy_bin "eve/.pio/build/eve_s3/firmware.bin" "eve.bin"
copy_bin "audio_esp/.pio/build/esp32-s3-devkitc-1/firmware.bin" "audio_esp.bin"
copy_bin "dock_station/.pio/build/dock_esp32/firmware.bin" "dock.bin"
copy_bin "wall_e_master_controller/.pio/build/cyd_esp32_2432s028/firmware.bin" "master_controller.bin"
copy_bin "vision_node/.pio/build/vision_s3/firmware.bin" "vision_node.bin"
copy_bin "ghostbusters_slime_blower/.pio/build/esp32-s3-devkitc-1/firmware.bin" "ghostbusters.bin"
echo "Done."
