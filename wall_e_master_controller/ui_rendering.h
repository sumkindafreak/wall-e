// ============================================================
//  UI rendering coordinator — delegates to ui_draw + laser overlay
// ============================================================

#pragma once

#include <TFT_eSPI.h>
#include "ui_state.h"

void uiRenderingInit(TFT_eSPI* tft);
void uiRenderingDrawDriveLaserOverlayIfNeeded(void);
