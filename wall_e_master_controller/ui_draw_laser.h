// ============================================================
//  Laser pad + crosshair drawing (Drive page, CYD)
// ============================================================

#pragma once

#include <TFT_eSPI.h>

void uiDrawLaserPadFrame(TFT_eSPI* tft);
void uiDrawLaserCrosshairDynamic(TFT_eSPI* tft, uint8_t aimPan, uint8_t aimTilt, bool armed);
