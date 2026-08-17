// ============================================================
// WALL-E Base Brain — PlatformIO entry wrapper
//
// The one implementation of setup()/loop() remains in main/main.ino so
// Arduino IDE / Arduino CLI and PlatformIO build the same robot firmware.
// PlatformIO compiles this normal C++ translation unit to avoid nested .ino
// preprocessing/linker behaviour.
// ============================================================

#include "main/main.ino"
