/**
 * EVE — layer-based LVGL tree for ONE physical eye panel.
 */
#pragma once

#include "config.h"

#if EVE_ENABLE_EYES

#include <lvgl.h>
#include "eve_eye.h"

/** Layer objects: background → glow → sclera → lids → highlight → scan. */
typedef struct {
  lv_obj_t* root;
  lv_obj_t* background;
  lv_obj_t* glow;
  lv_obj_t* sclera;
  lv_obj_t* highlight;
  lv_obj_t* lid_upper;
  lv_obj_t* lid_lower;
  lv_obj_t* scan_bar;
} EveSingleEyeUi;

void eveSingleEyeRendererInit(lv_obj_t* screen, EveSingleEyeUi* ui, EveEyeSide side);
void eveSingleEyeRendererApply(EveSingleEyeUi* ui, const EveEyeVisual* v);

#endif /* EVE_ENABLE_EYES */
