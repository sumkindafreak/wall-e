#pragma once

// ============================================================
//  WALL-E Web Server Header
// ============================================================

void webServerInit();
void webServerHandle();

/** Web UI is overriding autonomy: non-zero /drive, /stop cleared, or sticky "take over" from /api/autonomy/manual. */
bool webServerIsManualOverrideActive();
