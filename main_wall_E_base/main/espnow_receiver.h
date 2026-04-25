#pragma once

// ============================================================
//  WALL-E ESP-NOW Receiver
//  Receives ControlPacket from CYD controller, drives motors
//  Sends TelemetryPacket back to controller
// ============================================================

#include <Arduino.h>

void espnowReceiverInit();  // Call once in setup(), after WiFi init
void espnowSendTelemetry(); // Call periodically (e.g., every 100ms) to update controller UI

// NEW: Check if manual control is active
bool espnowIsManualControlActive();

/** Last values echoed to CYD (ack bits + last accepted seq) — for /api living JSON */
void espnowReceiverGetCydCommsForApi(uint16_t* lastSeq, uint8_t* ackBits);
