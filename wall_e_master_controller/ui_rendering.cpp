// ============================================================
//  UI rendering coordinator
// ============================================================

#include "ui_rendering.h"
#include "ui_draw.h"
#include "ui_draw_laser.h"
#include "cyd_laser_ui.h"

static TFT_eSPI* s_tft = nullptr;

void uiRenderingInit(TFT_eSPI* tft) {
  s_tft = tft;
}

void uiRenderingDrawDriveLaserOverlayIfNeeded(void) {
  if (!s_tft) return;
  if (g_currentPage != PAGE_DRIVE) return;
#if USE_PHYSICAL_JOYSTICKS
  if (g_inputMode != INPUT_PHYSICAL_JOYSTICK) return;
#endif
  uiDrawLaserButton(s_tft, cydLaserUiGetArmed());
}
