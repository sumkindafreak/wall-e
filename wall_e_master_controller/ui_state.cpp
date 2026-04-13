// ============================================================
//  WALL-E Master Controller — UI State Machine Implementation
// ============================================================

#include "ui_state.h"

InputMode         g_inputMode = INPUT_TOUCHSCREEN;
Page              g_currentPage = PAGE_DRIVE;
ControlAuthority  g_controlAuthority = CTRL_LOCAL;
uint8_t           g_motionPolicyFromBrain = 0;
bool              g_policyDenyCyd = false;
bool              g_estop = false;
bool              g_advancedMode = false;
bool              g_overlayVisible = false;
bool              g_needStaticRedraw = true;
uint8_t           g_helpSection = 0;
uint8_t           g_behaviourAnimPage = 0;
bool              g_topBannerCollapsed = false;
uint8_t           g_autonomyUiTab = 0;
bool              g_remoteAutonomyArm = false;
uint8_t           g_auCloseCm = 40;
uint8_t           g_auInterestCm = 80;
uint8_t           g_auCuriosityPct = 50;
uint8_t           g_auBraveryPct = 50;
uint8_t           g_auEnergyPct = 50;
uint8_t           g_auRandomPct = 50;
bool              g_auWaypointFollow = false;

void uiStateInit(void) {
#if USE_PHYSICAL_JOYSTICKS
  g_inputMode = INPUT_PHYSICAL_JOYSTICK;
  g_currentPage = PAGE_BEHAVIOUR;
#else
  g_inputMode = INPUT_TOUCHSCREEN;
  g_currentPage = PAGE_DRIVE;
#endif
  g_controlAuthority = CTRL_LOCAL;
  g_motionPolicyFromBrain = 0;
  g_policyDenyCyd = false;
  g_estop = false;
  g_advancedMode = false;
  g_overlayVisible = false;
  g_needStaticRedraw = true;
  g_topBannerCollapsed = false;
  g_autonomyUiTab = 0;
  g_remoteAutonomyArm = false;
  g_auCloseCm = 40;
  g_auInterestCm = 80;
  g_auCuriosityPct = 50;
  g_auBraveryPct = 50;
  g_auEnergyPct = 50;
  g_auRandomPct = 50;
  g_auWaypointFollow = false;
  g_helpSection = 0;
  g_behaviourAnimPage = 0;
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
