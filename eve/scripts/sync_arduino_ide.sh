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

# EVE_Companion.ino — same logic as main.cpp, Arduino IDE include path for awareness
INO="$SKETCH/EVE_Companion.ino"
cat > "$INO" << 'HEADER'
/**
 * EVE companion node — Arduino IDE sketch (ESP32-S3, same logic as PlatformIO eve/)
 *
 * Hand link: Serial1 @ GPIO17 TX / GPIO18 RX to WALL-E (see config.h).
 *
 * Libraries (Library Manager): ArduinoJson 6.x; lvgl 9.x; GFX Library for Arduino (moononournation).
 * Battery/current: Adafruit INA219 + Adafruit BusIO (when EVE_BATTERY_INA219 in config.h).
 * Optional ToF: Pololu VL53L1X. walle_i2s_wav_player.* bundled in this folder.
 *
 * Board: ESP32S3 Dev Module (16 MB flash, OPI PSRAM in Tools menu).
 * Regenerate this tree: bash scripts/sync_arduino_ide.sh
 */
HEADER

# main.cpp body with awareness include path for IDE subfolder
sed 's|#include "eve_awareness.h"|#include "awareness/eve_awareness.h"|' "$ROOT/src/main.cpp" >> "$INO"

echo "[sync] wrote EVE_Companion.ino from src/main.cpp"
echo "Done. Open arduino_ide/EVE_Companion/EVE_Companion.ino in Arduino IDE."
