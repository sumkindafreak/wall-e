/**
 * EVE face — owns LVGL object tree for visor, eyes, glow, lids, scanlines.
 */
#pragma once

#include "config.h"

#if EVE_ENABLE_EYES

#include <lvgl.h>
#include "eve_expression_state.h"

typedef struct {
  lv_obj_t* root;
  lv_obj_t* visor;
  lv_obj_t* eye_l;
  lv_obj_t* eye_r;
  lv_obj_t* glow_l;
  lv_obj_t* glow_r;
  lv_obj_t* lid_l;
  lv_obj_t* lid_r;
  lv_obj_t* scan_bar;
  lv_obj_t* dbg_label;
} EveEyeUi;

void eveEyeRendererInit(lv_obj_t* screen, EveEyeUi* ui);
void eveEyeRendererApply(EveEyeUi* ui, const EveEyeTarget* merged);

#if EVE_FACE_DEBUG_BENCH
void eveEyeRendererSetDebugText(EveEyeUi* ui, const char* txt);
#endif

#endif /* EVE_ENABLE_EYES */
