// ============================================================
//  WALL-E Master Controller — UI State Machine Implementation
// ============================================================

#include "ui_state.h"

InputMode         g_inputMode = INPUT_TOUCHSCREEN;
Page              g_currentPage = PAGE_DRIVE;
ControlAuthority  g_controlAuthority = CTRL_LOCAL;
bool              g_estop = false;
bool              g_advancedMode = false;
bool              g_overlayVisible = false;
bool              g_needStaticRedraw = true;

void uiStateInit(void) {
#if USE_PHYSICAL_JOYSTICKS
  g_inputMode = INPUT_PHYSICAL_JOYSTICK;
  g_currentPage = PAGE_BEHAVIOUR;
#else
  g_inputMode = INPUT_TOUCHSCREEN;
  g_currentPage = PAGE_DRIVE;
#endif
  g_controlAuthority = CTRL_LOCAL;
  g_estop = false;
  g_advancedMode = false;
  g_overlayVisible = false;
  g_needStaticRedraw = true;
}

bool uiStateIsTouchscreen(void) {
  return g_inputMode == INPUT_TOUCHSCREEN;
}

bool uiStateShowDrivePage(void) {
  return (g_inputMode == INPUT_TOUCHSCREEN && g_currentPage == PAGE_DRIVE);
}

bool uiStateShowVirtualJoysticks(void) {
  return uiStateShowDrivePage() && !g_overlayVisible;
}
