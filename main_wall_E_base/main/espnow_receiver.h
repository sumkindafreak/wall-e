#pragma once

// ============================================================
// WALL-E Base radio receiver compatibility API
//
// The historical espnow* names are retained so higher-level WALL-E code
// does not care whether the Base uses native ESP-NOW or the ESP32-P4
// radio-gateway transport.
// ============================================================

#include <Arduino.h>

void espnowReceiverInit();
void espnowReceiverHandle(); // Call every loop; required by the P4 UART bridge.
void espnowSendTelemetry();  // Call periodically (10 Hz in the Base loop).

bool espnowIsManualControlActive();
