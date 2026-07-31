#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_single_eye_renderer.h"
#include <math.h>
#include <string.h>

static void style_fill(lv_obj_t* o, lv_color_t c, lv_opa_t opa) {
  lv_obj_remove_style_all(o);
  lv_obj_set_style_bg_color(o, c, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(o, opa, LV_PART_MAIN);
  lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

void eveSingleEyeRendererInit(lv_obj_t* screen, EveSingleEyeUi* ui, EveEyeSide side) {
  (void)side;
  memset(ui, 0, sizeof(*ui));

  lv_display_t* disp = lv_display_get_default();
  int32_t dw = disp ? lv_display_get_horizontal_resolution(disp) : EVE_FACE_LCD_HOR_RES;
  int32_t dh = disp ? lv_display_get_vertical_resolution(disp) : EVE_FACE_LCD_VER_RES;

  ui->root = lv_obj_create(screen);
  lv_obj_set_size(ui->root, dw, dh);
  lv_obj_set_align(ui->root, LV_ALIGN_TOP_LEFT);
  lv_obj_remove_style_all(ui->root);
  style_fill(ui->root, lv_color_hex(0x020203), LV_OPA_COVER);

  ui->background = lv_obj_create(ui->root);
  lv_obj_set_size(ui->background, lv_pct(100), lv_pct(100));
  lv_obj_center(ui->background);
  style_fill(ui->background, lv_color_hex(0x060608), LV_OPA_COVER);
  lv_obj_set_style_radius(ui->background, 36, LV_PART_MAIN);

  ui->scan_bar = lv_obj_create(ui->background);
  style_fill(ui->scan_bar, lv_color_hex(0xaaccff), LV_OPA_30);
  lv_obj_set_size(ui->scan_bar, lv_pct(100), 3);
  lv_obj_align(ui->scan_bar, LV_ALIGN_TOP_MID, 0, 20);

  ui->glow = lv_obj_create(ui->background);
  style_fill(ui->glow, lv_color_hex(0x1a3a88), LV_OPA_40);
  lv_obj_set_size(ui->glow, 62, 88);
  lv_obj_center(ui->glow);
  lv_obj_set_style_radius(ui->glow, 31, LV_PART_MAIN);

  ui->sclera = lv_obj_create(ui->background);
  style_fill(ui->sclera, lv_color_hex(0x3a7cff), LV_OPA_COVER);
  lv_obj_set_size(ui->sclera, 52, 76);
  lv_obj_center(ui->sclera);
  lv_obj_set_style_radius(ui->sclera, 26, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui->sclera, 18, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(ui->sclera, lv_color_hex(0x2050cc), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(ui->sclera, LV_OPA_40, LV_PART_MAIN);

  ui->highlight = lv_obj_create(ui->sclera);
  style_fill(ui->highlight, lv_color_hex(0x9ec5ff), LV_OPA_60);
  lv_obj_set_size(ui->highlight, 12, 10);
  lv_obj_align(ui->highlight, LV_ALIGN_TOP_LEFT, 10, 12);
  lv_obj_set_style_radius(ui->highlight, 6, LV_PART_MAIN);

  ui->lid_upper = lv_obj_create(ui->sclera);
  lv_obj_remove_style_all(ui->lid_upper);
  lv_obj_set_size(ui->lid_upper, lv_pct(100), 0);
  lv_obj_align(ui->lid_upper, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(ui->lid_upper, lv_color_hex(0x050506), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->lid_upper, LV_OPA_COVER, LV_PART_MAIN);

  ui->lid_lower = lv_obj_create(ui->sclera);
  lv_obj_remove_style_all(ui->lid_lower);
  lv_obj_set_size(ui->lid_lower, lv_pct(100), 0);
  lv_obj_align(ui->lid_lower, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(ui->lid_lower, lv_color_hex(0x050506), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->lid_lower, LV_OPA_COVER, LV_PART_MAIN);
}

void eveSingleEyeRendererApply(EveSingleEyeUi* ui, const EveEyeVisual* v) {
  if (!ui || !v) {
    return;
  }

  int32_t bh = (int32_t)(76 * v->scaleY);
  int32_t bw = (int32_t)(52 * v->scaleX);
  int32_t gh = (int32_t)(88 * v->scaleY);
  int32_t gw = (int32_t)(62 * v->scaleX);

  lv_obj_set_size(ui->sclera, bw, bh);
  lv_obj_set_size(ui->glow, gw, gh);
  lv_obj_set_style_radius(ui->sclera, bw / 2, LV_PART_MAIN);
  lv_obj_set_style_radius(ui->glow, gw / 2, LV_PART_MAIN);

  int32_t vw = lv_obj_get_width(ui->background);
  int32_t vh = lv_obj_get_height(ui->background);
  float gx = v->gazeX * (vw * 0.14f);
  float gy = v->gazeY * (vh * 0.12f);
  lv_obj_align(ui->sclera, LV_ALIGN_CENTER, (int32_t)gx, (int32_t)gy);
  lv_obj_align(ui->glow, LV_ALIGN_CENTER, (int32_t)gx, (int32_t)gy);

  lv_opa_t glow = (lv_opa_t)constrain((int)(v->glowOpa * v->brightness), 0, 255);
  lv_obj_set_style_bg_opa(ui->glow, (lv_opa_t)(glow * 40 / 255), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(ui->sclera, (lv_opa_t)(glow / 3), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->scan_bar, (lv_opa_t)constrain((int)v->scanOpa, 0, 255), LV_PART_MAIN);

  int32_t lidUp = (int32_t)(bh * v->lidUpper);
  int32_t lidLo = (int32_t)(bh * v->lidLower);
  if (lidUp > bh) {
    lidUp = bh;
  }
  if (lidLo > bh) {
    lidLo = bh;
  }
  lv_obj_set_height(ui->lid_upper, lidUp);
  lv_obj_set_height(ui->lid_lower, lidLo);

  int32_t tilt = (int32_t)(v->tiltDeg * 10);
  lv_obj_set_style_transform_rotation(ui->background, tilt, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_x(ui->background, vw / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(ui->background, vh / 2, LV_PART_MAIN);
}

#endif /* EVE_ENABLE_EYES */
