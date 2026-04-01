// ============================================================
//  Touch handling facade — wraps XPT2046 + zone decode
// ============================================================

#pragma once

#include "touch_input.h"

static inline void touchHandlingInit(void) {
  touchInit();
}

static inline TouchZone touchHandlingUpdate(int page) {
  return touchUpdate(page);
}
