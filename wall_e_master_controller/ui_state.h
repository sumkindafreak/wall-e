// ============================================================
//  WALL-E Master Controller — UI State Machine
//  InputMode, Page, ControlAuthority — centralised state
// ============================================================

#ifndef UI_STATE_H
#define UI_STATE_H

#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------
//  Compile-time: physical joysticks (future)
// ------------------------------------------------------------
#define USE_PHYSICAL_JOYSTICKS 1  // Physical joysticks ENABLED

// ------------------------------------------------------------
//  Input Mode
// ------------------------------------------------------------
typedef enum {
  INPUT_TOUCHSCREEN,
  INPUT_PHYSICAL_JOYSTICK
} InputMode;

// ------------------------------------------------------------
//  Page
// ------------------------------------------------------------
typedef enum {
  PAGE_DRIVE,
  PAGE_BEHAVIOUR,
  PAGE_SYSTEM,
  PAGE_PROFILE,       // Profile selection
  PAGE_SERVO_EDITOR,  // Per-profile servo tuning
  PAGE_SERVO_TEST,    // Individual servo testing
  PAGE_AUTONOMY,      // NEW: Autonomy status & control
  PAGE_WAYPOINTS,     // NEW: Waypoint management
  PAGE_HELP,          // Help topics (from System)
  PAGE_SD_EXPLORER    // SD card file browser
} Page;

// ------------------------------------------------------------
//  Control Authority (display indicator)
// ------------------------------------------------------------
typedef enum {
  CTRL_LOCAL,       // Green
  CTRL_AUTONOMOUS,  // Blue
  CTRL_SUPERVISED,  // Yellow
  CTRL_SAFETY       // Red (override/E-STOP)
} ControlAuthority;

// ------------------------------------------------------------
//  Eye State (animation)
// ------------------------------------------------------------
typedef enum {
  EYE_NORMAL,
  EYE_BLINKING,
  EYE_TIRED,    // Narrow
  EYE_EXCITED,  // Bounce
  EYE_ESTOP     // Flash wide
} EyeState;

// ------------------------------------------------------------
//  Global UI State (set by main loop)
// ------------------------------------------------------------
extern InputMode     g_inputMode;
extern Page          g_currentPage;
extern ControlAuthority g_controlAuthority;
extern bool          g_estop;
extern bool          g_advancedMode;   // Triple-tap top-left
extern bool          g_overlayVisible; // Long-press quick action
extern bool          g_needStaticRedraw;
/** Tap top banner (full or mini strip) to toggle — frees vertical space for content */
extern bool          g_topBannerCollapsed;

/** PAGE_AUTONOMY: 0 = live telemetry, 1 = remote tuning */
extern uint8_t       g_autonomyUiTab;
/** When true, outgoing packets set FLAG_AUTONOMOUS (Base runs autonomy engine) */
extern bool          g_remoteAutonomyArm;
/** Edited tuning values (0-100 for traits; cm for distances) — Tune tab */
extern uint8_t       g_auCloseCm;
extern uint8_t       g_auInterestCm;
extern uint8_t       g_auCuriosityPct;
extern uint8_t       g_auBraveryPct;
extern uint8_t       g_auEnergyPct;
extern uint8_t       g_auRandomPct;
extern bool          g_auWaypointFollow;

/** PAGE_HELP: 0 = topic list, 1..4 = topic body */
extern uint8_t       g_helpSection;

// ------------------------------------------------------------
//  Init — call from setup()
// ------------------------------------------------------------
void uiStateInit(void);

// ------------------------------------------------------------
//  Query helpers
// ------------------------------------------------------------
bool uiStateIsTouchscreen(void);
bool uiStateShowDrivePage(void);   // True if touchscreen AND on drive
bool uiStateShowVirtualJoysticks(void);

#endif // UI_STATE_H
