#!/usr/bin/env bash
# Sync PlatformIO eve/ sources into arduino_ide/EVE_Companion/ for Arduino IDE builds.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCH="$ROOT/arduino_ide/EVE_Companion"
AWARE="$SKETCH/awareness"

mkdir -p "$AWARE"

echo "[sync] include/*.h -> EVE_Companion/"
cp "$ROOT/include/"*.h "$SKETCH/"

echo "[sync] src/*.cpp (except main) -> EVE_Companion/"
for f in "$ROOT/src/"*.cpp; do
  base="$(basename "$f")"
  if [[ "$base" == "main.cpp" ]]; then
    continue
  fi
  cp "$f" "$SKETCH/$base"
done

echo "[sync] awareness headers + eve_awareness.cpp"
cp "$ROOT/awareness/"*.h "$AWARE/"
cp "$ROOT/src/awareness/eve_awareness.cpp" "$AWARE/"

echo "[sync] walle_i2s_audio library"
bash "$ROOT/scripts/sync_arduino_i2s_lib.sh"

# EVE_Companion.ino — main.cpp body + Arduino awareness include path
INO="$SKETCH/EVE_Companion.ino"
cat > "$INO" << 'HEADER'
/**
 * EVE companion node — Arduino IDE (ESP32-S3 N16R8)
 *
 * Same logic as PlatformIO eve/src/main.cpp. Regenerate: bash scripts/sync_arduino_ide.sh
 *
 * Hand link: Serial1 @ GPIO17 TX / GPIO18 RX (config.h).
 * Libraries: ArduinoJson 6.x; Adafruit INA219/BusIO; lvgl+GFX (eyes); VL53L1X (ToF).
 * Bundled: walle_i2s_wav_player.* (SD → WAV → I2S; no DFPlayer).
 *
 * Board: ESP32S3 Dev Module — 16 MB flash, OPI PSRAM in Tools menu.
 */
HEADER
awk '/^#include/ {found=1} found' "$ROOT/src/main.cpp" \
  | sed 's|#include "eve_awareness.h"|#include "awareness/eve_awareness.h"|' >> "$INO"

echo "[sync] wrote EVE_Companion.ino from src/main.cpp"
echo "Done. Open arduino_ide/EVE_Companion/EVE_Companion.ino in Arduino IDE."
