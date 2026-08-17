// ============================================================
// WALL-E CYD — Arduino-ESP32 ESP header compatibility shim
//
// Some older CYD source files include <ESP.h>. On the Arduino-ESP32 core
// used by PlatformIO the canonical header is <Esp.h> (case-sensitive on
// Linux CI). Keep this tiny shim so Arduino IDE/Windows and Linux CI build
// exactly the same source without changing the developer-console logic.
// ============================================================

#ifndef WALLE_CYD_ESP_COMPAT_H
#define WALLE_CYD_ESP_COMPAT_H

#include <Esp.h>

#endif  // WALLE_CYD_ESP_COMPAT_H
