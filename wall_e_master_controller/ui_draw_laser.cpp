// ============================================================
//  Laser pad visuals
// ============================================================

#include "ui_draw_laser.h"
#include "cyd_laser_ui.h"
#include "ui_draw.h"

void uiDrawLaserPadFrame(TFT_eSPI* tft) {
  if (!tft) return;
  tft->drawRect(CYD_LASER_PAD_X, CYD_LASER_PAD_Y, CYD_LASER_PAD_W, CYD_LASER_PAD_H, C_ACCENT);
  tft->setTextColor(C_ACCENT, C_BG);
  tft->setTextSize(1);
  tft->drawString("LASER AIM", CYD_LASER_PAD_X + 4, CYD_LASER_PAD_Y + 4);
  tft->drawRect(CYD_LASER_FIRE_X, CYD_LASER_FIRE_Y, CYD_LASER_FIRE_W, CYD_LASER_FIRE_H, C_YELLOW);
  tft->setTextColor(C_YELLOW, C_BG);
  tft->drawString("FIRE", CYD_LASER_FIRE_X + 6, CYD_LASER_FIRE_Y + 6);
}

void uiDrawLaserCrosshairDynamic(TFT_eSPI* tft, uint8_t aimPan, uint8_t aimTilt, bool armed) {
  if (!tft) return;
  int cx = CYD_LASER_PAD_X + (aimPan * (CYD_LASER_PAD_W - 1)) / 100;
  int cy = CYD_LASER_PAD_Y + (aimTilt * (CYD_LASER_PAD_H - 1)) / 100;
  uint16_t col = armed ? C_RED : C_ACCENT_DIM;
  tft->drawCircle(cx, cy, 5, col);
  tft->drawFastHLine(cx - 8, cy, 17, col);
  tft->drawFastVLine(cx, cy - 8, 17, col);
}
