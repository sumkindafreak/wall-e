#pragma once

#include <cstdint>

// ============================================================
//  WALL-E Dock Homing
//  RSSI-based homing toward dock beacon. Call from loop when active.
// ============================================================

/* Feed beacon reception (call from ESP-NOW recv when DockBeaconPacket received) */
void dockHomingOnBeacon(int8_t rssi);

/* Set/clear "go to dock" request */
void dockHomingSetRequested(bool requested);
bool dockHomingIsRequested(void);

/* Returns true if homing is active and producing motor output */
bool dockHomingIsActive(void);

/* Get motor output. Returns true if valid output; fills left, right (-255..255) */
bool dockHomingGetMotorOutput(int16_t *left, int16_t *right);

/* Call every loop. Returns true if homing consumed this cycle (use its motor output) */
bool dockHomingUpdate(uint32_t now);

/* Stray detection: call every loop. Auto-triggers homing when dock beacon is lost
   or RSSI weak for too long (WALL-E strayed too far). Returns true if auto-triggered. */
void dockHomingCheckAutoReturn(uint32_t now);
