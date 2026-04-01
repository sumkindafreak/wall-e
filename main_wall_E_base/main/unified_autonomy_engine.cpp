// ============================================================
//  WALL-E Unified Autonomy Engine — implementation
// ============================================================

#include "unified_autonomy_engine.h"
#include "autonomy_engine.h"
#include "autonomous_docking.h"
#include "battery_monitor.h"
#include "dock_config.h"
#if !USE_AUTONOMOUS_DOCKING
#include "dock_homing.h"
#endif
#include "imu_manager.h"
#include "sonar_sensor.h"

#include <Arduino.h>

static UnifiedAutonomyState s_uaState = UA_IDLE;
static bool s_safetyActive = false;
static bool s_safetyLatched = false;

static bool s_everHadSonar = false;
static uint32_t s_lastSonarOkMs = 0;

static const char* uaStateName(UnifiedAutonomyState s) {
  switch (s) {
    case UA_IDLE: return "IDLE";
    case UA_EXPLORE: return "EXPLORE";
    case UA_AVOID_OBSTACLE: return "AVOID_OBSTACLE";
    case UA_SEEK_DOCK: return "SEEK_DOCK";
    case UA_DOCKING: return "DOCKING";
    case UA_CHARGING: return "CHARGING";
    case UA_MANUAL_OVERRIDE: return "MANUAL_OVERRIDE";
    case UA_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

static bool computeSafety(uint32_t now) {
  if (sonarIsValid()) {
    s_everHadSonar = true;
    s_lastSonarOkMs = now;
  }
  const bool sonarLost =
      s_everHadSonar && (now - s_lastSonarOkMs > SONAR_TIMEOUT_MS);

  const ImuData& imu = imuGetData();
  const bool tiltBad =
      isIMUCalibrated() && imu.valid && imu.tiltAlert;

  const BatteryData& bat = batteryGetData();
  const bool overcurrent =
      bat.valid && (bat.currentA > UA_OVERCURRENT_A);

  return sonarLost || tiltBad || overcurrent;
}

static bool isExploreLike(AutoState st) {
  switch (st) {
    case AUTO_EXPLORE_LOOP:
    case AUTO_SCAN:
    case AUTO_WANDER:
    case AUTO_NAVIGATE_WAYPOINT:
    case AUTO_RETURN_HOME:
    case AUTO_APPROACH:
    case AUTO_TRACK_OBJECT:
    case AUTO_EVALUATE:
    case AUTO_ORIENT:
    case AUTO_INVESTIGATE_HEIGHT:
      return true;
    default:
      return false;
  }
}

void unifiedAutonomyInit(void) {
  s_uaState = UA_IDLE;
  s_safetyActive = false;
  s_safetyLatched = false;
  s_everHadSonar = false;
  s_lastSonarOkMs = 0;
}

void unifiedAutonomyTick(uint32_t now, bool manualControlActive) {
  const bool safety = computeSafety(now);
  s_safetyActive = safety;

  if (safety) {
    if (!s_safetyLatched) {
      autonomyEmergencyStop();
      s_safetyLatched = true;
    }
    s_uaState = UA_ERROR;
    return;
  }
  s_safetyLatched = false;

  if (manualControlActive) {
    s_uaState = UA_MANUAL_OVERRIDE;
    return;
  }

#if USE_AUTONOMOUS_DOCKING
  {
    const DockState ds = autonomousDockingGetState();
    if (ds == DOCK_STATE_CHARGING) {
      s_uaState = UA_CHARGING;
      return;
    }
    if (ds == DOCK_STATE_SEARCH) {
      s_uaState = UA_SEEK_DOCK;
      return;
    }
    if (ds == DOCK_STATE_ALIGN || ds == DOCK_STATE_APPROACH || ds == DOCK_STATE_DOCKED) {
      s_uaState = UA_DOCKING;
      return;
    }
  }
#else
  if (dockHomingIsActive()) {
    s_uaState = UA_DOCKING;
    return;
  }
#endif

  const AutoState st = autonomyGetState();
  if (st == AUTO_AVOID || st == AUTO_REACT) {
    s_uaState = UA_AVOID_OBSTACLE;
    return;
  }
  if (isExploreLike(st)) {
    s_uaState = UA_EXPLORE;
    return;
  }

  s_uaState = UA_IDLE;
}

UnifiedAutonomyState unifiedAutonomyGetState(void) {
  return s_uaState;
}

const char* unifiedAutonomyGetStateName(void) {
  return uaStateName(s_uaState);
}

bool unifiedAutonomySafetyActive(void) {
  return s_safetyActive;
}

namespace WallEAutonomy {

bool isObstacleAhead(void) {
  return sonarIsValid() && sonarIsObjectClose(DETECT_CLOSE_CM);
}

bool isBatteryLow(void) {
  const BatteryData& b = batteryGetData();
  if (!b.valid) {
    return false;
  }
  return b.percent < DOCK_BATTERY_LOW_PCT;
}

bool isDockDetected(void) {
#if USE_AUTONOMOUS_DOCKING
  const DockState ds = autonomousDockingGetState();
  return ds != DOCK_STATE_IDLE;
#else
  return dockHomingIsActive() || dockHomingIsRequested();
#endif
}

bool isCharging(void) {
#if USE_AUTONOMOUS_DOCKING
  return autonomousDockingGetState() == DOCK_STATE_CHARGING;
#else
  return false;
#endif
}

}  // namespace WallEAutonomy
