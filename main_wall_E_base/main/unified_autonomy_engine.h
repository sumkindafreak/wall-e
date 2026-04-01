// ============================================================
//  WALL-E Unified Autonomy Engine — single high-level state
//
//  Orchestrates existing subsystems (autonomy_engine, autonomous_docking,
//  sonar, battery, IMU) without reimplementing their internals.
//  Call unifiedAutonomyTick() once per loop after autonomousDockingUpdate().
// ============================================================

#ifndef UNIFIED_AUTONOMY_ENGINE_H
#define UNIFIED_AUTONOMY_ENGINE_H

#include <Arduino.h>
#include <stdint.h>

// Stall / overcurrent on 12V line (amps). Tune for your drivetrain.
#ifndef UA_OVERCURRENT_A
#define UA_OVERCURRENT_A 15.0f
#endif

typedef enum {
  UA_IDLE = 0,
  UA_EXPLORE,
  UA_AVOID_OBSTACLE,
  UA_SEEK_DOCK,
  UA_DOCKING,
  UA_CHARGING,
  UA_MANUAL_OVERRIDE,
  UA_ERROR
} UnifiedAutonomyState;

void unifiedAutonomyInit(void);

// Call every loop after sonarUpdate, autonomyUpdate, autonomousDockingUpdate (or legacy homing).
// manualControlActive: CYD / web manual (same signal as autonomySetManualOverride).
void unifiedAutonomyTick(uint32_t now, bool manualControlActive);

UnifiedAutonomyState unifiedAutonomyGetState(void);
const char* unifiedAutonomyGetStateName(void);

// True while safety rules demand no autonomous/dock drive (tilt, sonar loss, overcurrent).
bool unifiedAutonomySafetyActive(void);

// Thin wrappers — call existing modules only (for tests / telemetry / integration).
namespace WallEAutonomy {
bool isObstacleAhead(void);
bool isBatteryLow(void);
bool isDockDetected(void);
bool isCharging(void);
}

#endif
