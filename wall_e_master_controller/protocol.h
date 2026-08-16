#pragma once

// ============================================================
// WALL-E Master Controller protocol compatibility header
//
// The byte-level CYD <-> Base contract now lives in one shared file.
// Keeping this wrapper means the rest of the controller can continue to
// include "protocol.h" without knowing the repository layout changed.
// ============================================================

#include "../protocols/walle_control_protocol.h"
