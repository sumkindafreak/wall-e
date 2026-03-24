/*******************************************************************************
 * autonomous_docking.cpp
 * WALL-E Autonomous Docking — State Machine
 *
 * Implements full navigation and docking behaviour:
 * IDLE → (battery low) → SEARCH → ALIGN → APPROACH → DOCKED → CHARGING
 ******************************************************************************/

#include "autonomous_docking.h"
#include "battery_monitor.h"
#include "dock_sensors.h"
#include "dock_controller.h"
#include "dock_protocol.h"
#include "vl53l1x_tof.h"
#include "ir_beacon_receivers.h"
#include "motor_control.h"
#include "display_manager.h"
#include <Arduino.h>

/* Speed levels for approach phase */
#define SPEED_NORMAL    140
#define SPEED_SLOW      80
#define SPEED_CRAWL     40
#define SPEED_SEEK      70
#define SPEED_ALIGN     60
#define RSSI_THRESHOLD  -80
#define RSSI_GOOD       -60

static DockState       s_state = DOCK_STATE_IDLE;
static bool            s_requested = false;
static int8_t          s_lastRssi = -100;
static uint32_t        s_lastBeaconMs = 0;
static uint32_t        s_stateEnterMs = 0;
static uint32_t        s_lastDockId = 0;
static uint32_t        s_lastApproachStageMs = 0;
static int16_t         s_outLeft = 0, s_outRight = 0;
static uint8_t         s_irHint = DOCK_IR_ALIGN_LOST;
static uint32_t        s_lastIrHintMs = 0;

#define APPROACH_STAGE_SEND_MS  150

/* Motor output abstraction — replace with your real motor API if needed */
static void driveForward(int16_t speed) {
  s_outLeft = speed;
  s_outRight = speed;
}
static void slowForward(int16_t speed) {
  s_outLeft = speed;
  s_outRight = speed;
}
static void crawlForward(int16_t speed) {
  s_outLeft = speed;
  s_outRight = speed;
}
static void rotateSlow(int16_t left, int16_t right) {
  s_outLeft = left;
  s_outRight = right;
}
static void steerLeft(int16_t baseSpeed) {
  s_outLeft = baseSpeed / 2;
  s_outRight = baseSpeed;
}
static void steerRight(int16_t baseSpeed) {
  s_outLeft = baseSpeed;
  s_outRight = baseSpeed / 2;
}
static void stopMotors(void) {
  s_outLeft = 0;
  s_outRight = 0;
  motorStop();
}

void autonomousDockingInit(void) {
  s_state = DOCK_STATE_IDLE;
  s_requested = false;
  s_lastRssi = -100;
  s_lastBeaconMs = 0;
  s_stateEnterMs = 0;
  s_lastDockId = 0;
}

void autonomousDockingOnBeacon(int8_t rssi) {
  s_lastRssi = rssi;
  s_lastBeaconMs = millis();
  /* dock_id comes from beacon packet — stored by espnow_receiver when parsing */
  /* We use 0 for broadcast; dock listens for REQUEST_CHARGE */
}

/* Store dock_id when we receive beacon (call from espnow callback if you have it) */
void autonomousDockingSetLastDockId(uint32_t dock_id) {
  s_lastDockId = dock_id;
}

void autonomousDockingOnIrAlign(uint8_t ir_align_hint) {
  s_irHint = ir_align_hint;
  s_lastIrHintMs = millis();
}

static bool irAlignHintFresh(uint32_t now) {
  return (now - s_lastIrHintMs) < 500u;
}

void autonomousDockingSetRequested(bool requested) {
  s_requested = requested;
  if (requested && s_state == DOCK_STATE_IDLE) {
    s_state = DOCK_STATE_SEARCH;
    s_stateEnterMs = millis();
  }
  if (!requested && s_state != DOCK_STATE_CHARGING) {
    s_state = DOCK_STATE_IDLE;
    stopMotors();
  }
}

static bool batteryLow(void) {
  const BatteryData& b = batteryGetData();
  if (!b.valid) return false;
  return b.percent < DOCK_BATTERY_LOW_PCT;
}

static bool batteryOk(void) {
  const BatteryData& b = batteryGetData();
  if (!b.valid) return true;
  return b.percent >= DOCK_BATTERY_OK_PCT;
}

bool autonomousDockingUpdate(uint32_t now) {
  /* IDLE: Check for low battery → auto-start search */
  if (s_state == DOCK_STATE_IDLE) {
    irBeaconSetTransmitEnabled(false);
    if (batteryLow() && !s_requested) {
      s_requested = true;
      s_state = DOCK_STATE_SEARCH;
      s_stateEnterMs = now;
      displayShowToast("Battery low - searching for dock");
    }
    if (s_requested) {
      s_state = DOCK_STATE_SEARCH;
      s_stateEnterMs = now;
    }
    return false;
  }

  /* CHARGING: No motor output */
  if (s_state == DOCK_STATE_CHARGING) {
    irBeaconSetTransmitEnabled(false);
    stopMotors();
    return false;
  }

  {
    bool wantTx = (s_state == DOCK_STATE_SEARCH || s_state == DOCK_STATE_ALIGN ||
                   s_state == DOCK_STATE_APPROACH);
    irBeaconSetTransmitEnabled(wantTx);
  }

  /* Default: no output */
  s_outLeft = 0;
  s_outRight = 0;

  /* Abort conditions */
  if (s_state != DOCK_STATE_IDLE && s_state != DOCK_STATE_CHARGING) {
    if (s_lastBeaconMs > 0 && (now - s_lastBeaconMs) > DOCK_BEACON_TIMEOUT_MS) {
      s_state = DOCK_STATE_IDLE;
      s_requested = false;
      stopMotors();
      displayShowToast("Dock search timeout");
      return false;
    }
    if ((now - s_stateEnterMs) > DOCK_APPROACH_TIMEOUT_MS) {
      s_state = DOCK_STATE_IDLE;
      s_requested = false;
      stopMotors();
      displayShowToast("Dock approach timeout");
      return false;
    }
  }

  /* -----------------------------------------------------------------------
   * SEARCH_DOCK: Rotate slowly, listen for ESP-NOW beacon or IR
   * ----------------------------------------------------------------------- */
  if (s_state == DOCK_STATE_SEARCH) {
    if (now - s_lastApproachStageMs >= APPROACH_STAGE_SEND_MS) {
      dockControllerSendApproachStage(APPROACH_FAR, s_lastDockId ? s_lastDockId : 0);
      s_lastApproachStageMs = now;
    }
    if (s_lastRssi > RSSI_THRESHOLD) {
      /* Strong beacon: run ALIGN so dock IR + beacon hint can center the robot */
      s_state = DOCK_STATE_ALIGN;
      s_stateEnterMs = now;
      return true;
    }
    /* Rotate left slowly */
    rotateSlow(-SPEED_SEEK, SPEED_SEEK);
    return true;
  }

  /* -----------------------------------------------------------------------
   * ALIGN_BEACON: Steer until left ≈ right IR
   * ----------------------------------------------------------------------- */
  if (s_state == DOCK_STATE_ALIGN) {
    if (now - s_lastApproachStageMs >= APPROACH_STAGE_SEND_MS) {
      uint8_t stage = APPROACH_1M;
      if (tofIsValid()) {
        uint16_t d = tofGetDistanceMm();
        if (d > 0 && d <= 200) stage = APPROACH_20CM;
        else if (d > 1000) stage = APPROACH_FAR;
      }
      dockControllerSendApproachStage(stage, s_lastDockId ? s_lastDockId : 0);
      s_lastApproachStageMs = now;
    }
    if (!irAlignHintFresh(now)) {
      /* No recent dock packet with IR hint — creep and weave */
      rotateSlow(-SPEED_ALIGN / 2, SPEED_ALIGN);
      return true;
    }

    switch (s_irHint) {
      case DOCK_IR_ALIGN_CENTER:
        s_state = DOCK_STATE_APPROACH;
        s_stateEnterMs = now;
        driveForward(SPEED_NORMAL);
        return true;
      case DOCK_IR_ALIGN_LEFT:
        steerLeft(SPEED_ALIGN);
        return true;
      case DOCK_IR_ALIGN_RIGHT:
        steerRight(SPEED_ALIGN);
        return true;
      case DOCK_IR_ALIGN_PAUSE:
        stopMotors();
        return true;
      case DOCK_IR_ALIGN_LOST:
      default:
        rotateSlow(-SPEED_ALIGN / 2, SPEED_ALIGN);
        return true;
    }
  }

  /* -----------------------------------------------------------------------
   * APPROACH: Drive forward, ToF-based speed
   * ----------------------------------------------------------------------- */
  if (s_state == DOCK_STATE_APPROACH) {
    if (now - s_lastApproachStageMs >= APPROACH_STAGE_SEND_MS) {
      uint8_t stage = APPROACH_1M;
      if (tofIsValid()) {
        uint16_t d = tofGetDistanceMm();
        if (d > 0 && d < DOCK_TOF_SLOW_MM) stage = APPROACH_20CM;  /* precision to beam */
        else if (d < 1000) stage = APPROACH_1M;
        else stage = APPROACH_FAR;
      }
      dockControllerSendApproachStage(stage, s_lastDockId ? s_lastDockId : 0);
      s_lastApproachStageMs = now;
    }
    /* Arrival: IR beam or ToF < 60 mm */
    if (dockBeamPresent()) {
      s_state = DOCK_STATE_DOCKED;
      s_stateEnterMs = now;
      stopMotors();
      return true;
    }
    if (tofIsValid()) {
      uint16_t d = tofGetDistanceMm();
      if (d > 0 && d < DOCK_TOF_CRAWL_MM) {
        s_state = DOCK_STATE_DOCKED;
        s_stateEnterMs = now;
        stopMotors();
        return true;
      }
    }

    /* Speed from distance */
    int16_t spd = SPEED_NORMAL;
    if (tofIsValid()) {
      uint16_t d = tofGetDistanceMm();
      if (d > 0) {
        if (d < DOCK_TOF_CRAWL_MM)       spd = 0;
        else if (d < DOCK_TOF_SLOW_MM)   spd = SPEED_CRAWL;
        else if (d < DOCK_TOF_NORMAL_MM) spd = SPEED_SLOW;
      }
    }
    if (spd == 0) {
      s_state = DOCK_STATE_DOCKED;
      s_stateEnterMs = now;
      stopMotors();
      return true;
    }
    driveForward(spd);
    return true;
  }

  /* -----------------------------------------------------------------------
   * DOCKED: Send REQUEST_CHARGE → CHARGING
   * ----------------------------------------------------------------------- */
  if (s_state == DOCK_STATE_DOCKED) {
    dockControllerSendRequestCharge(s_lastDockId);
    s_state = DOCK_STATE_CHARGING;
    s_requested = false;
    stopMotors();
    displayShowToast("Charging...");
    return false;
  }

  return false;
}

bool autonomousDockingGetMotorOutput(int16_t* left, int16_t* right) {
  if (!left || !right) return false;
  *left  = s_outLeft;
  *right = s_outRight;
  return (s_outLeft != 0 || s_outRight != 0);
}

bool autonomousDockingIsActive(void) {
  return s_state != DOCK_STATE_IDLE && s_state != DOCK_STATE_CHARGING;
}

DockState autonomousDockingGetState(void) {
  return s_state;
}

const char* autonomousDockingGetStateName(void) {
  switch (s_state) {
    case DOCK_STATE_SEARCH: return "SEARCHING";
    case DOCK_STATE_ALIGN: return "ALIGNING";
    case DOCK_STATE_APPROACH: return "APPROACH";
    case DOCK_STATE_DOCKED: return "DOCKED";
    case DOCK_STATE_CHARGING: return "CHARGING";
    case DOCK_STATE_IDLE:
    default: return "IDLE";
  }
}
