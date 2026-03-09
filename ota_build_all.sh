#!/bin/bash
# WALL-E OTA Build Script (Linux/macOS)
# Builds all OTA-enabled projects. Run from wall-e repo root.
# Usage: ./ota_build_all.sh
# Upload: ./ota_build_all.sh upload

UPLOAD="${1:-}"

projects=(
  "main_wall_E_base:wall_e_brain_s3:Base"
  "wall_e_master_controller:cyd_esp32_2432s028:Controller"
  "dock_station:dock_esp32:Dock"
)

for entry in "${projects[@]}"; do
  IFS=':' read -r path env name <<< "$entry"
  if [ ! -f "$path/platformio.ini" ]; then
    echo "[$name] Skipping (no platformio.ini)"
    continue
  fi
  echo ""
  echo "[$name] Building $path..."
  (cd "$path" && pio run -e "$env") || exit 1
  if [ "$UPLOAD" = "upload" ]; then
    echo "[$name] Uploading (OTA)..."
    (cd "$path" && pio run -e "$env" -t upload) || exit 1
  fi
done
echo ""
echo "Done."
