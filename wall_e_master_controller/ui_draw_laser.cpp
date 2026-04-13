// ============================================================
//  Laser toggle button (Drive page, CYD)
// ============================================================

#include "ui_draw_laser.h"
#include "cyd_laser_ui.h"
#include "ui_draw.h"

void uiDrawLaserButton(TFT_eSPI* tft, bool armed) {
  if (!tft) return;
  uint16_t bg = armed ? C_RED : C_BG_DARK;
  uint16_t border = armed ? C_WHITE : C_BORDER;
  uint16_t fg = armed ? C_WHITE : C_ACCENT;
  tft->fillRect(CYD_LASER_BTN_X, CYD_LASER_BTN_Y, CYD_LASER_BTN_W, CYD_LASER_BTN_H, bg);
  tft->drawRect(CYD_LASER_BTN_X, CYD_LASER_BTN_Y, CYD_LASER_BTN_W, CYD_LASER_BTN_H, border);
  tft->setTextColor(fg, bg);
  tft->setTextSize(1);
  tft->drawString(armed ? "LASER ON" : "LASER OFF", CYD_LASER_BTN_X + 10, CYD_LASER_BTN_Y + 6);
  tft->setTextColor(C_TEXT_DIM, bg);
  tft->drawString(armed ? "tap: off" : "tap: on", CYD_LASER_BTN_X + 10, CYD_LASER_BTN_Y + 22);
}
