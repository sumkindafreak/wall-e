/**
 * EVE — dual SPI eye panels (separate chip selects, shared bus).
 */
#pragma once

#include "config.h"

#if EVE_ENABLE_EYES

#include <lvgl.h>
#include <stdint.h>

typedef enum {
  EVE_EYE_PANEL_LEFT = 0,
  EVE_EYE_PANEL_RIGHT,
  EVE_EYE_PANEL_COUNT,
} EveEyePanelId;

void eveEyeDisplayInit(void);
bool eveEyeDisplayIsDualPhysical(void);
lv_display_t* eveEyeDisplayGet(EveEyePanelId panel);
void eveEyeDisplayTickLvgl(uint32_t dtMs);

#endif /* EVE_ENABLE_EYES */
