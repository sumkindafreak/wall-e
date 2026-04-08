#pragma once

#include <stdint.h>
#include "motor_control.h"

// ============================================================
//  Central motion / high-level state layer
//  Single place for: drive profile hints, speed caps, turn feel,
//  allowed motion, servo/head hints (consumers read getters).
//  Does NOT replace motorStop / failsafe / CYD manual profile rules.
// ============================================================

typedef enum : uint8_t {
  MOTION_MODE_EMERGENCY = 0,       // unified safety latched / fault
  MOTION_MODE_IDLE_STANDBY,        // powered, no autonomy, no manual
  MOTION_MODE_MANUAL,              // CYD or WebUI has priority
  MOTION_MODE_AUTONOMY_ROAM,       // general autonomous driving
  MOTION_MODE_OBSTACLE_AVOID,      // careful avoidance
  MOTION_MODE_EXPRESSIVE_REACT,    // AUTO_REACT / theatrical motion
  MOTION_MODE_DOCK_SEARCH,         // FSM: look for beacon
  MOTION_MODE_DOCK_ALIGN,          // FSM: IR / fine steering
  MOTION_MODE_DOCK_APPROACH,       // FSM: ToF approach
  MOTION_MODE_DOCK_CHARGING,       // docked / charging (no drive)
  MOTION_MODE_DOCK_LEGACY_HOMING,  // RSSI legacy homing (no fine FSM)
} MotionLayerMode;

/** Optional hint for higher-level animation / servo policy (emotion, pose bridge, etc.) */
typedef enum : uint8_t {
  SERVO_LAYER_NEUTRAL = 0,
  SERVO_LAYER_OPERATOR,
  SERVO_LAYER_AUTONOMY,
  SERVO_LAYER_DOCKING,
  SERVO_LAYER_ALERT,
  SERVO_LAYER_EXPRESSIVE,
} ServoLayerProfile;

/** Head / eyes while base is moving — vision & pose can blend with this */
typedef enum : uint8_t {
  MOTION_HEAD_DEFAULT = 0,
  MOTION_HEAD_TRACK_VISION,
  MOTION_HEAD_FORWARD_AHEAD,
  MOTION_HEAD_AWAY_IDLE,
} MotionHeadHint;

#define MOTION_MASK_FORWARD  0x01u
#define MOTION_MASK_REVERSE  0x02u
#define MOTION_MASK_TURN     0x04u
#define MOTION_MASK_STRAFE   0x08u
#define MOTION_MASK_ALL      0x0Fu

typedef struct {
  MotionLayerMode mode;
  DriveProfile driveProfile;
  uint8_t speedCapPct;        // 0–100 applied to autonomy tank commands
  uint8_t turnSharpnessPct;   // 0–100 scales turn differential (100 = full)
  uint32_t allowedMotionMask;
  ServoLayerProfile servoProfile;
  MotionHeadHint headHint;
} MotionLayerSnapshot;

void motionLayerInit(void);

/**
 * Call once per loop after dock/autonomy updates, with current control flags.
 * When NOT manual: applies drive profile via motor subsystem (no spam if unchanged).
 * Manual: does not override CYD/WebUI drive profile (they set per command).
 */
void motionLayerUpdate(uint32_t now, bool manualControlActive, bool safetyActive);

const MotionLayerSnapshot* motionLayerGetSnapshot(void);
MotionLayerMode motionLayerGetMode(void);
const char* motionLayerGetModeName(MotionLayerMode mode);
const char* motionLayerGetModeNameCurrent(void);

ServoLayerProfile motionLayerGetServoProfile(void);
MotionHeadHint motionLayerGetHeadHint(void);
uint32_t motionLayerGetAllowedMotionMask(void);

/** After autonomy outputs -255..255, apply layer speed cap + turn sharpness */
void motionLayerApplyMotorTankLimits(int16_t* left, int16_t* right);
