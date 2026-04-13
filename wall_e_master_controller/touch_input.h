// ============================================================
//  WALL-E Master Controller — Touch Input
//  Proper virtual joystick math: deadzone, radial clamping, smoothing
// ============================================================

#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include "protocol.h"
#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// ------------------------------------------------------------
//  Touch zones (for page nav)
// ------------------------------------------------------------
typedef enum {
  TOUCH_ZONE_NONE,
  TOUCH_ZONE_LEFT_JOY,
  TOUCH_ZONE_RIGHT_JOY,
  TOUCH_ZONE_ESTOP,
  TOUCH_ZONE_NAV_BEHAV,
  TOUCH_ZONE_NAV_SYSTEM,
  TOUCH_ZONE_NAV_BACK,
  TOUCH_ZONE_NAV_PROFILE,      // Navigate to Profile page
  TOUCH_ZONE_NAV_SERVO_EDIT,   // Navigate to Servo Editor
  TOUCH_ZONE_NAV_SERVO_TEST,   // Navigate to Servo Test page
  TOUCH_ZONE_QUICK_ACTION,     // Long-press 2s bottom-right
  TOUCH_ZONE_ADVANCED,         // Triple-tap top-left
  TOUCH_ZONE_PROFILE_0,        // Profile card 0
  TOUCH_ZONE_PROFILE_1,        // Profile card 1
  TOUCH_ZONE_PROFILE_2,        // Profile card 2
  TOUCH_ZONE_PROFILE_EDIT_0,   // Edit button for profile 0
  TOUCH_ZONE_PROFILE_EDIT_1,   // Edit button for profile 1
  TOUCH_ZONE_PROFILE_EDIT_2,   // Edit button for profile 2
  TOUCH_ZONE_SERVO_SAVE,       // Save servo settings
  TOUCH_ZONE_SERVO_RESET,      // Reset servo settings
  TOUCH_ZONE_SLIDER_0,         // Head sensitivity slider
  TOUCH_ZONE_SLIDER_1,         // Servo speed slider
  TOUCH_ZONE_SLIDER_2,         // Deadzone slider
  TOUCH_ZONE_SLIDER_3,         // Expo slider
  TOUCH_ZONE_SLIDER_4,         // Max speed slider
  TOUCH_ZONE_SERVO_SLIDER_0,   // Individual servo 0 slider
  TOUCH_ZONE_SERVO_SLIDER_1,   // Individual servo 1 slider
  TOUCH_ZONE_SERVO_SLIDER_2,   // Individual servo 2 slider
  TOUCH_ZONE_SERVO_SLIDER_3,   // Individual servo 3 slider
  TOUCH_ZONE_SERVO_SLIDER_4,   // Individual servo 4 slider
  TOUCH_ZONE_SERVO_SLIDER_5,   // Individual servo 5 slider
  TOUCH_ZONE_SERVO_SLIDER_6,   // Individual servo 6 slider
  TOUCH_ZONE_SERVO_SLIDER_7,   // Individual servo 7 slider
  TOUCH_ZONE_SERVO_SLIDER_8,   // Individual servo 8 slider
  TOUCH_ZONE_SERVO_SLIDER_9,   // Individual servo 9 slider
  TOUCH_ZONE_SERVO_SAVE_NEUTRAL, // Save current positions as neutral
  TOUCH_ZONE_SERVO_NEUTRAL,    // Set all servos to neutral
  TOUCH_ZONE_SERVO_TEST1,      // Test pose 1
  TOUCH_ZONE_SERVO_TEST2,      // Test pose 2
  TOUCH_ZONE_MOOD_CURIOUS,     // Behaviour: Curious mood
  TOUCH_ZONE_MOOD_HAPPY,       // Behaviour: Happy mood
  TOUCH_ZONE_MOOD_SHY,         // Behaviour: Shy mood
  TOUCH_ZONE_MOOD_TIRED,       // Behaviour: Tired mood
  TOUCH_ZONE_MOOD_EXCITED,     // Behaviour: Excited mood
  TOUCH_ZONE_MOOD_FAV6,        // Drive: 6th favorite / mood slot
  TOUCH_ZONE_ANIM_0,           // Animation 0 (Reset)
  TOUCH_ZONE_ANIM_1,           // Animation 1 (Bootup)
  TOUCH_ZONE_ANIM_2,           // Animation 2 (Inquisitive)
  TOUCH_ZONE_ANIM_3,           // Animation 3 (EyebrowRight)
  TOUCH_ZONE_ANIM_4,           // Animation 4 (EyebrowLeft)
  TOUCH_ZONE_ANIM_5,           // Animation 5 (Surprised)
  TOUCH_ZONE_ANIM_6,
  TOUCH_ZONE_ANIM_7,
  TOUCH_ZONE_ANIM_8,
  TOUCH_ZONE_ANIM_9,
  TOUCH_ZONE_ANIM_10,
  TOUCH_ZONE_ANIM_11,
  TOUCH_ZONE_ANIM_12,
  TOUCH_ZONE_ANIM_13,
  TOUCH_ZONE_ANIM_14,
  TOUCH_ZONE_ANIM_15,
  TOUCH_ZONE_ANIM_16,
  TOUCH_ZONE_ANIM_17,
  TOUCH_ZONE_ANIM_18,
  TOUCH_ZONE_ANIM_19,
  TOUCH_ZONE_ANIM_20,
  TOUCH_ZONE_ANIM_21,
  TOUCH_ZONE_ANIM_22,
  TOUCH_ZONE_ANIM_23,
  TOUCH_ZONE_BEHAV_PAGE,       // Cycle Behaviour animation pages (4×6 grid)
  TOUCH_ZONE_NAV_AUTONOMY,     // NEW: Navigate to Autonomy page
  TOUCH_ZONE_NAV_HELP,         // System page → Help
  TOUCH_ZONE_HELP_TOPIC_0,
  TOUCH_ZONE_HELP_TOPIC_1,
  TOUCH_ZONE_HELP_TOPIC_2,
  TOUCH_ZONE_HELP_TOPIC_3,
  TOUCH_ZONE_NAV_SD,           // System page → Memory Core
  TOUCH_ZONE_SYS_MOTION_POLICY, // System page — tap to cycle motion policy (→ Base)
  TOUCH_ZONE_MC_SC_0,          // Shortcuts Mem..Evt (PAGE_SD_EXPLORER)
  TOUCH_ZONE_MC_SC_1,
  TOUCH_ZONE_MC_SC_2,
  TOUCH_ZONE_MC_SC_3,
  TOUCH_ZONE_MC_SC_4,
  TOUCH_ZONE_MC_SC_5,
  TOUCH_ZONE_SD_LIST_0,        // +0..3 visible rows
  TOUCH_ZONE_SD_LIST_1,
  TOUCH_ZONE_SD_LIST_2,
  TOUCH_ZONE_SD_LIST_3,
  TOUCH_ZONE_SD_UP,
  TOUCH_ZONE_SD_OPEN,
  TOUCH_ZONE_SD_REFRESH,
  TOUCH_ZONE_SD_PG_PREV,
  TOUCH_ZONE_SD_PG_NEXT,
  TOUCH_ZONE_SD_DELETE,
  TOUCH_ZONE_SD_CONFIRM_YES,
  TOUCH_ZONE_SD_CONFIRM_NO,
  TOUCH_ZONE_SD_RENAME,        // Memory Core toolbar — rename file
  TOUCH_ZONE_SD_RENAME_CH_DEC,
  TOUCH_ZONE_SD_RENAME_CH_INC,
  TOUCH_ZONE_SD_RENAME_BKSP,
  TOUCH_ZONE_SD_RENAME_CUR_L,
  TOUCH_ZONE_SD_RENAME_CUR_R,
  TOUCH_ZONE_SD_RENAME_CANCEL,
  TOUCH_ZONE_SD_RENAME_OK,
  TOUCH_ZONE_DOCK_GO,          // Go to dock (sends ACTION_DOCK_GO)
  TOUCH_ZONE_DOCK_CANCEL,      // Cancel dock homing (sends ACTION_DOCK_CANCEL)
  TOUCH_ZONE_LASER_TOGGLE,     // Laser beam on/off (head pose follows robot; no screen aim)
  TOUCH_ZONE_AUTONOMY_TAB_LIVE,
  TOUCH_ZONE_AUTONOMY_TAB_TUNE,
  TOUCH_ZONE_AUTONOMY_ARM,
  TOUCH_ZONE_AUTONOMY_M_CLOSE,
  TOUCH_ZONE_AUTONOMY_P_CLOSE,
  TOUCH_ZONE_AUTONOMY_M_INT,
  TOUCH_ZONE_AUTONOMY_P_INT,
  TOUCH_ZONE_AUTONOMY_M_CUR,
  TOUCH_ZONE_AUTONOMY_P_CUR,
  TOUCH_ZONE_AUTONOMY_M_BRV,
  TOUCH_ZONE_AUTONOMY_P_BRV,
  TOUCH_ZONE_AUTONOMY_WAYPOINT,
  TOUCH_ZONE_AUTONOMY_PRESET_0,
  TOUCH_ZONE_AUTONOMY_PRESET_1,
  TOUCH_ZONE_AUTONOMY_PRESET_2,
  TOUCH_ZONE_AUTONOMY_PRESET_3
} TouchZone;

#define LONG_PRESS_MS  2000
#define TRIPLE_TAP_WINDOW_MS  800

// Page indices (0=Drive, 1=Behaviour, 2=System — pass as int)
// ------------------------------------------------------------
//  Touch config (CYD XPT2046)
// ------------------------------------------------------------
#define TOUCH_MIN_PRESSURE 200
#define TOUCH_X_MIN 200
#define TOUCH_X_MAX 3700
#define TOUCH_Y_MIN 240
#define TOUCH_Y_MAX 3800

// ------------------------------------------------------------
//  Joystick math
// ------------------------------------------------------------
#define JOY_DEADZONE_PCT  10    // 8-12% radial deadzone
#define JOY_SMOOTH_FACTOR 0.6f  // output = new*0.6 + prev*0.4

// ------------------------------------------------------------
//  API
// ------------------------------------------------------------
void touchInit(void);
TouchZone touchUpdate(int page);           // Call each loop — reads touch, updates drive state, returns zone
void touchSetOverlayDismissed(void);       // Clear long-press after overlay closed (optional)

void touchGetDriveState(DriveState* out);  // Smoothed output
void touchGetJoystickDots(int* joyX, int* joyY);  // For UI dot
unsigned long touchLastActivityMs(void);   // For safety lock (200ms → STOP)

TouchZone touchGetZone(int screenX, int screenY, int page);  // page: 0=Drive, 1=Behaviour, 2=System

XPT2046_Touchscreen* touchGetTs(void);
SPIClass* touchGetSPI(void);

#endif // TOUCH_INPUT_H
