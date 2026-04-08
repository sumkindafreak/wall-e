// ============================================================
//  Motion layer — central mode + motor/servo hints
// ============================================================

#include "motion_layer.h"
#include "autonomy_engine.h"
#include "dock_config.h"
#include "dock_homing.h"
#include "autonomous_docking.h"
#include <Arduino.h>
#include <math.h>

static MotionLayerSnapshot s_snap;
static MotionLayerMode s_lastMode = MOTION_MODE_IDLE_STANDBY;

static void fillSnapshot(MotionLayerMode mode, DriveProfile dp, uint8_t cap, uint8_t turnPct,
                           uint32_t mask, ServoLayerProfile sp, MotionHeadHint hh) {
  s_snap.mode = mode;
  s_snap.driveProfile = dp;
  s_snap.speedCapPct = cap;
  s_snap.turnSharpnessPct = turnPct;
  s_snap.allowedMotionMask = mask;
  s_snap.servoProfile = sp;
  s_snap.headHint = hh;
}

void motionLayerInit(void) {
  fillSnapshot(MOTION_MODE_IDLE_STANDBY, DRIVE_PROFILE_NORMAL, 100, 100, MOTION_MASK_ALL,
               SERVO_LAYER_NEUTRAL, MOTION_HEAD_DEFAULT);
  s_lastMode = MOTION_MODE_IDLE_STANDBY;
  Serial.println(F("[Motion] layer init (idle)"));
}

const char* motionLayerGetModeName(MotionLayerMode mode) {
  switch (mode) {
    case MOTION_MODE_EMERGENCY: return "EMERGENCY";
    case MOTION_MODE_IDLE_STANDBY: return "IDLE_STANDBY";
    case MOTION_MODE_MANUAL: return "MANUAL";
    case MOTION_MODE_AUTONOMY_ROAM: return "AUTONOMY_ROAM";
    case MOTION_MODE_OBSTACLE_AVOID: return "OBSTACLE_AVOID";
    case MOTION_MODE_EXPRESSIVE_REACT: return "EXPRESSIVE_REACT";
    case MOTION_MODE_DOCK_SEARCH: return "DOCK_SEARCH";
    case MOTION_MODE_DOCK_ALIGN: return "DOCK_ALIGN";
    case MOTION_MODE_DOCK_APPROACH: return "DOCK_APPROACH";
    case MOTION_MODE_DOCK_CHARGING: return "DOCK_CHARGING";
    case MOTION_MODE_DOCK_LEGACY_HOMING: return "DOCK_LEGACY_HOMING";
    default: return "?";
  }
}

const char* motionLayerGetModeNameCurrent(void) {
  return motionLayerGetModeName(s_snap.mode);
}

const MotionLayerSnapshot* motionLayerGetSnapshot(void) {
  return &s_snap;
}

MotionLayerMode motionLayerGetMode(void) {
  return s_snap.mode;
}

ServoLayerProfile motionLayerGetServoProfile(void) {
  return s_snap.servoProfile;
}

MotionHeadHint motionLayerGetHeadHint(void) {
  return s_snap.headHint;
}

uint32_t motionLayerGetAllowedMotionMask(void) {
  return s_snap.allowedMotionMask;
}

void motionLayerApplyMotorTankLimits(int16_t* left, int16_t* right) {
  if (!left || !right) return;
  uint8_t cap = s_snap.speedCapPct;
  uint8_t ts = s_snap.turnSharpnessPct;
  if (cap > 100) cap = 100;
  if (ts > 100) ts = 100;

  int32_t l = (int32_t)*left * (int32_t)cap / 100;
  int32_t r = (int32_t)*right * (int32_t)cap / 100;
  l = constrain(l, -255, 255);
  r = constrain(r, -255, 255);

  float avg = (float)(l + r) / 2.0f;
  float diff = (float)(r - l) / 2.0f;
  diff *= (float)ts / 100.0f;
  int32_t nl = (int32_t)lroundf(avg - diff);
  int32_t nr = (int32_t)lroundf(avg + diff);
  *left = (int16_t)constrain(nl, -255, 255);
  *right = (int16_t)constrain(nr, -255, 255);
}

void motionLayerUpdate(uint32_t now, bool manualControlActive, bool safetyActive) {
  (void)now;

  const uint32_t all = MOTION_MASK_ALL;

  if (safetyActive) {
    fillSnapshot(MOTION_MODE_EMERGENCY, DRIVE_PROFILE_NORMAL, 0, 100, 0,
                 SERVO_LAYER_ALERT, MOTION_HEAD_FORWARD_AHEAD);
    if (s_lastMode != MOTION_MODE_EMERGENCY) {
      Serial.printf("[Motion] mode -> %s (unified safety)\n", motionLayerGetModeName(MOTION_MODE_EMERGENCY));
      s_lastMode = MOTION_MODE_EMERGENCY;
    }
    if (!manualControlActive) {
      motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "motion: emergency hold");
    }
    return;
  }

  if (manualControlActive) {
    fillSnapshot(MOTION_MODE_MANUAL, DRIVE_PROFILE_NORMAL, 100, 100, all,
                 SERVO_LAYER_OPERATOR, MOTION_HEAD_DEFAULT);
    if (s_lastMode != MOTION_MODE_MANUAL) {
      Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_MANUAL));
      s_lastMode = MOTION_MODE_MANUAL;
    }
    return;
  }

  /* --- Autonomous / background: choose mode + drive profile --- */
#if USE_AUTONOMOUS_DOCKING
  {
    /* IsActive() is false in CHARGING; still need mode for docked/charging UX */
    DockState ds = autonomousDockingGetState();
    if (ds == DOCK_STATE_CHARGING || ds == DOCK_STATE_DOCKED) {
      fillSnapshot(MOTION_MODE_DOCK_CHARGING, DRIVE_PROFILE_PRECISION, 0, 85, 0,
                   SERVO_LAYER_DOCKING, MOTION_HEAD_FORWARD_AHEAD);
      if (s_lastMode != MOTION_MODE_DOCK_CHARGING) {
        Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_DOCK_CHARGING));
        s_lastMode = MOTION_MODE_DOCK_CHARGING;
      }
      motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: dock charging/docked");
      return;
    }
    if (autonomousDockingIsActive()) {
    if (ds == DOCK_STATE_SEARCH) {
      fillSnapshot(MOTION_MODE_DOCK_SEARCH, DRIVE_PROFILE_PRECISION, 80, 90, all,
                   SERVO_LAYER_DOCKING, MOTION_HEAD_FORWARD_AHEAD);
      if (s_lastMode != MOTION_MODE_DOCK_SEARCH) {
        Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_DOCK_SEARCH));
        s_lastMode = MOTION_MODE_DOCK_SEARCH;
      }
      motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: dock search");
      return;
    }
    if (ds == DOCK_STATE_ALIGN) {
      fillSnapshot(MOTION_MODE_DOCK_ALIGN, DRIVE_PROFILE_PRECISION, 65, 75, MOTION_MASK_FORWARD | MOTION_MASK_TURN,
                   SERVO_LAYER_DOCKING, MOTION_HEAD_FORWARD_AHEAD);
      if (s_lastMode != MOTION_MODE_DOCK_ALIGN) {
        Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_DOCK_ALIGN));
        s_lastMode = MOTION_MODE_DOCK_ALIGN;
      }
      motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: dock align");
      return;
    }
    if (ds == DOCK_STATE_APPROACH) {
      fillSnapshot(MOTION_MODE_DOCK_APPROACH, DRIVE_PROFILE_PRECISION, 70, 80,
                   MOTION_MASK_FORWARD | MOTION_MASK_TURN, SERVO_LAYER_DOCKING, MOTION_HEAD_FORWARD_AHEAD);
      if (s_lastMode != MOTION_MODE_DOCK_APPROACH) {
        Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_DOCK_APPROACH));
        s_lastMode = MOTION_MODE_DOCK_APPROACH;
      }
      motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: dock approach");
      return;
    }
    fillSnapshot(MOTION_MODE_DOCK_SEARCH, DRIVE_PROFILE_PRECISION, 75, 90, all,
                 SERVO_LAYER_DOCKING, MOTION_HEAD_FORWARD_AHEAD);
    motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: dock (active)");
    if (s_lastMode != MOTION_MODE_DOCK_SEARCH) {
      Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_DOCK_SEARCH));
      s_lastMode = MOTION_MODE_DOCK_SEARCH;
    }
    return;
    }
  }
#else
  if (dockHomingIsActive()) {
    fillSnapshot(MOTION_MODE_DOCK_LEGACY_HOMING, DRIVE_PROFILE_PRECISION, 72, 88, all,
                 SERVO_LAYER_DOCKING, MOTION_HEAD_FORWARD_AHEAD);
    if (s_lastMode != MOTION_MODE_DOCK_LEGACY_HOMING) {
      Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_DOCK_LEGACY_HOMING));
      s_lastMode = MOTION_MODE_DOCK_LEGACY_HOMING;
    }
    motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: legacy homing");
    return;
  }
#endif

  if (autonomyIsEnabled()) {
    AutoState ast = autonomyGetState();
    if (ast == AUTO_AVOID) {
      fillSnapshot(MOTION_MODE_OBSTACLE_AVOID, DRIVE_PROFILE_PRECISION, 78, 82,
                   MOTION_MASK_FORWARD | MOTION_MASK_REVERSE | MOTION_MASK_TURN,
                   SERVO_LAYER_AUTONOMY, MOTION_HEAD_FORWARD_AHEAD);
      if (s_lastMode != MOTION_MODE_OBSTACLE_AVOID) {
        Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_OBSTACLE_AVOID));
        s_lastMode = MOTION_MODE_OBSTACLE_AVOID;
      }
      motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: obstacle avoid");
      return;
    }
    if (ast == AUTO_APPROACH || ast == AUTO_ORIENT) {
      fillSnapshot(MOTION_MODE_AUTONOMY_ROAM, DRIVE_PROFILE_PRECISION, 82, 88, all,
                   SERVO_LAYER_AUTONOMY, MOTION_HEAD_TRACK_VISION);
      if (s_lastMode != MOTION_MODE_AUTONOMY_ROAM) {
        Serial.printf("[Motion] mode -> %s (careful)\n", motionLayerGetModeName(MOTION_MODE_AUTONOMY_ROAM));
        s_lastMode = MOTION_MODE_AUTONOMY_ROAM;
      }
      motorSetDriveProfile(DRIVE_PROFILE_PRECISION, "motion: approach/orient");
      return;
    }
    if (ast == AUTO_REACT) {
      fillSnapshot(MOTION_MODE_EXPRESSIVE_REACT, DRIVE_PROFILE_EXPRESSIVE, 92, 95, all,
                   SERVO_LAYER_EXPRESSIVE, MOTION_HEAD_TRACK_VISION);
      if (s_lastMode != MOTION_MODE_EXPRESSIVE_REACT) {
        Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_EXPRESSIVE_REACT));
        s_lastMode = MOTION_MODE_EXPRESSIVE_REACT;
      }
      motorSetDriveProfile(DRIVE_PROFILE_EXPRESSIVE, "motion: expressive react");
      return;
    }
    fillSnapshot(MOTION_MODE_AUTONOMY_ROAM, DRIVE_PROFILE_NORMAL, 88, 95, all,
                 SERVO_LAYER_AUTONOMY, MOTION_HEAD_TRACK_VISION);
    if (s_lastMode != MOTION_MODE_AUTONOMY_ROAM) {
      Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_AUTONOMY_ROAM));
      s_lastMode = MOTION_MODE_AUTONOMY_ROAM;
    }
    motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "motion: autonomy roam");
    return;
  }

  fillSnapshot(MOTION_MODE_IDLE_STANDBY, DRIVE_PROFILE_NORMAL, 100, 100, all,
               SERVO_LAYER_NEUTRAL, MOTION_HEAD_AWAY_IDLE);
  if (s_lastMode != MOTION_MODE_IDLE_STANDBY) {
    Serial.printf("[Motion] mode -> %s\n", motionLayerGetModeName(MOTION_MODE_IDLE_STANDBY));
    s_lastMode = MOTION_MODE_IDLE_STANDBY;
  }
  motorSetDriveProfile(DRIVE_PROFILE_NORMAL, "motion: idle standby");
}
