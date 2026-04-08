// ============================================================
//  WALL-E CYD — Confirm dialog draw
// ============================================================

#include "ui_confirm_dialog.h"
#include "ui_draw.h"

#include <TFT_eSPI.h>
#include <Arduino.h>
#include <string.h>

void uiConfirmDrawDeleteFile(TFT_eSPI* tft, const char* filename) {
  if (!tft) return;
  tft->fillScreen(C_BG);
  tft->setTextColor(C_RED, C_BG);
  tft->setTextSize(2);
  tft->drawString("Delete file?", 8, 40);
  tft->setTextSize(1);
  tft->setTextColor(C_TEXT_DIM, C_BG);
  if (filename && filename[0]) {
    char line[52];
    strncpy(line, filename, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    if (strlen(line) > 44) {
      line[41] = '.';
      line[42] = '.';
      line[43] = '.';
      line[44] = '\0';
    }
    tft->setCursor(8, 72);
    tft->print(line);
  }
  tft->fillRect(40, 120, 100, 36, C_RED);
  tft->setTextColor(C_WHITE, C_RED);
  tft->drawString("YES", 72, 132);
  tft->fillRect(180, 120, 100, 36, C_BG_DARK);
  tft->drawRect(180, 120, 100, 36, C_BORDER);
  tft->setTextColor(C_ACCENT, C_BG_DARK);
  tft->drawString("NO", 218, 132);
}
