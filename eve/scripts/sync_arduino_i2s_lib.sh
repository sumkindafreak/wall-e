#!/usr/bin/env bash
# Copy shared I2S WAV library into Arduino IDE sketch + libraries folder.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/../lib/walle_i2s_audio"
DEST_SKETCH="$ROOT/arduino_ide/EVE_Companion"
DEST_LIB="$ROOT/arduino_ide/libraries/walle_i2s_audio"

cp "$SRC/src/walle_i2s_wav_player.h" "$SRC/src/walle_i2s_wav_player.cpp" "$DEST_SKETCH/"
mkdir -p "$DEST_LIB/src"
cp "$SRC/library.properties" "$DEST_LIB/"
cp "$SRC/src/walle_i2s_wav_player.h" "$SRC/src/walle_i2s_wav_player.cpp" "$DEST_LIB/src/"
echo "Synced walle_i2s_audio -> arduino_ide (sketch tabs + libraries/)"
