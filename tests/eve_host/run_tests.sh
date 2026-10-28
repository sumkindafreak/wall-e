#!/usr/bin/env bash
# Host (PC) tests for EVE's link / safety / routine logic and WALL-E's EVE peer link.
# These run the real firmware modules with a simulated UART wire and a simulated clock.
# They do NOT test hardware: no servo, LED, speaker, display or battery is involved.
#
# Usage: tests/eve_host/run_tests.sh            (needs g++ and git, or ARDUINOJSON_DIR)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
SKETCH="$ROOT/eve/arduino_ide/EVE_Companion"
BASE="$ROOT/main_wall_E_base/main"

if [ -z "${ARDUINOJSON_DIR:-}" ]; then
  ARDUINOJSON_DIR="$HERE/.deps/ArduinoJson"
  if [ ! -f "$ARDUINOJSON_DIR/src/ArduinoJson.h" ]; then
    mkdir -p "$HERE/.deps"
    git clone -q --depth 1 --branch v6.21.5 https://github.com/bblanchon/ArduinoJson.git "$ARDUINOJSON_DIR"
  fi
fi

mkdir -p "$HERE/build"
g++ -std=c++17 -Wall -Wextra -Werror -O1 -g \
  -I"$HERE/shim" -I"$ARDUINOJSON_DIR/src" -I"$SKETCH" -I"$BASE" \
  "$HERE/test_eve_link.cpp" \
  "$SKETCH/eve_control_core.cpp" "$SKETCH/eve_safety.cpp" "$SKETCH/eve_routines.cpp" \
  "$SKETCH/eve_battery_logic.cpp" "$SKETCH/eve_pin_check.cpp" \
  "$BASE/eve_peer_link.cpp" \
  -o "$HERE/build/test_eve_link"
"$HERE/build/test_eve_link" "$@"
