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
#define USE_PHYSICAL_JOYSTICKS 0

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
  PAGE_SYSTEM
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
