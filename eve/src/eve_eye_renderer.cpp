#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye_renderer.h"
#include <math.h>
#include <string.h>

static void style_eye(lv_obj_t* o, lv_color_t c, lv_opa_t opa) {
  lv_obj_remove_style_all(o);
  lv_obj_set_style_bg_color(o, c, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(o, opa, LV_PART_MAIN);
  lv_obj_set_style_border_width(o, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

void eveEyeRendererInit(lv_obj_t* screen, EveEyeUi* ui) {
  memset(ui, 0, sizeof(*ui));

  ui->root = lv_obj_create(screen);
  lv_display_t* disp = lv_display_get_default();
  int32_t dw = disp ? lv_display_get_horizontal_resolution(disp) : EVE_FACE_LCD_HOR_RES;
  int32_t dh = disp ? lv_display_get_vertical_resolution(disp) : EVE_FACE_LCD_VER_RES;
  lv_obj_set_size(ui->root, dw, dh);
  lv_obj_set_align(ui->root, LV_ALIGN_TOP_LEFT);
  lv_obj_remove_style_all(ui->root);
  lv_obj_set_style_bg_color(ui->root, lv_color_hex(0x020203), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->root, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(ui->root, LV_OBJ_FLAG_SCROLLABLE);

  ui->visor = lv_obj_create(ui->root);
  lv_obj_remove_style_all(ui->visor);
  lv_obj_set_size(ui->visor, lv_pct(92), lv_pct(78));
  lv_obj_center(ui->visor);
  lv_obj_set_style_bg_color(ui->visor, lv_color_hex(0x060608), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->visor, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(ui->visor, 36, LV_PART_MAIN);
  lv_obj_clear_flag(ui->visor, LV_OBJ_FLAG_SCROLLABLE);

  ui->scan_bar = lv_obj_create(ui->visor);
  style_eye(ui->scan_bar, lv_color_hex(0xaaccff), LV_OPA_30);
  lv_obj_set_size(ui->scan_bar, lv_pct(100), 3);
  lv_obj_set_align(ui->scan_bar, LV_ALIGN_TOP_MID);
  lv_obj_set_y(ui->scan_bar, 24);

  ui->glow_l = lv_obj_create(ui->visor);
  style_eye(ui->glow_l, lv_color_hex(0x1a3a88), LV_OPA_40);
  lv_obj_set_size(ui->glow_l, 62, 88);
  lv_obj_set_align(ui->glow_l, LV_ALIGN_CENTER);
  lv_obj_set_x(ui->glow_l, -48);
  lv_obj_set_style_radius(ui->glow_l, 31, LV_PART_MAIN);

  ui->glow_r = lv_obj_create(ui->visor);
  style_eye(ui->glow_r, lv_color_hex(0x1a3a88), LV_OPA_40);
  lv_obj_set_size(ui->glow_r, 62, 88);
  lv_obj_set_align(ui->glow_r, LV_ALIGN_CENTER);
  lv_obj_set_x(ui->glow_r, 48);
  lv_obj_set_style_radius(ui->glow_r, 31, LV_PART_MAIN);

  ui->eye_l = lv_obj_create(ui->visor);
  style_eye(ui->eye_l, lv_color_hex(0x3a7cff), LV_OPA_COVER);
  lv_obj_set_size(ui->eye_l, 52, 76);
  lv_obj_set_align(ui->eye_l, LV_ALIGN_CENTER);
  lv_obj_set_x(ui->eye_l, -48);
  lv_obj_set_style_radius(ui->eye_l, 26, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui->eye_l, 18, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(ui->eye_l, lv_color_hex(0x2050cc), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(ui->eye_l, LV_OPA_40, LV_PART_MAIN);

  ui->eye_r = lv_obj_create(ui->visor);
  style_eye(ui->eye_r, lv_color_hex(0x3a7cff), LV_OPA_COVER);
  lv_obj_set_size(ui->eye_r, 52, 76);
  lv_obj_set_align(ui->eye_r, LV_ALIGN_CENTER);
  lv_obj_set_x(ui->eye_r, 48);
  lv_obj_set_style_radius(ui->eye_r, 26, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui->eye_r, 18, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(ui->eye_r, lv_color_hex(0x2050cc), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(ui->eye_r, LV_OPA_40, LV_PART_MAIN);

  ui->lid_l = lv_obj_create(ui->eye_l);
  lv_obj_remove_style_all(ui->lid_l);
  lv_obj_set_size(ui->lid_l, lv_pct(100), 0);
  lv_obj_set_align(ui->lid_l, LV_ALIGN_TOP_MID);
  lv_obj_set_style_bg_color(ui->lid_l, lv_color_hex(0x050506), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->lid_l, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(ui->lid_l, LV_OBJ_FLAG_SCROLLABLE);

  ui->lid_r = lv_obj_create(ui->eye_r);
  lv_obj_remove_style_all(ui->lid_r);
  lv_obj_set_size(ui->lid_r, lv_pct(100), 0);
  lv_obj_set_align(ui->lid_r, LV_ALIGN_TOP_MID);
  lv_obj_set_style_bg_color(ui->lid_r, lv_color_hex(0x050506), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->lid_r, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(ui->lid_r, LV_OBJ_FLAG_SCROLLABLE);

#if EVE_FACE_DEBUG_BENCH
  ui->dbg_label = lv_label_create(ui->root);
  lv_obj_set_style_text_color(ui->dbg_label, lv_color_hex(0x8899aa), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->dbg_label, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui->dbg_label, lv_color_hex(0x101018), LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui->dbg_label, 4, LV_PART_MAIN);
  lv_obj_align(ui->dbg_label, LV_ALIGN_BOTTOM_LEFT, 4, -4);
  lv_label_set_text(ui->dbg_label, "EVE face");
#endif
}

#if EVE_FACE_DEBUG_BENCH
void eveEyeRendererSetDebugText(EveEyeUi* ui, const char* txt) {
  if (ui && ui->dbg_label) {
    lv_label_set_text(ui->dbg_label, txt);
  }
}
#endif

void eveEyeRendererApply(EveEyeUi* ui, const EveEyeTarget* m) {
  if (!ui || !m) {
    return;
  }

  int32_t vw = lv_obj_get_width(ui->visor);
  int32_t vh = lv_obj_get_height(ui->visor);
  if (vw < 40 || vh < 40) {
    return;
  }

  float sep = m->eyeSep;
  float sx = m->eyeScaleX;
  float sy = m->eyeScaleY * (1.f - 0.45f * m->squint);
  int32_t bw = (int32_t)(52 * sx);
  int32_t bh = (int32_t)(76 * sy);
  int32_t gw = (int32_t)(62 * sx * 1.05f);
  int32_t gh = (int32_t)(88 * sy * 1.05f);
  int32_t gap = (int32_t)(42.f * sep);

  lv_obj_set_size(ui->eye_l, bw, bh);
  lv_obj_set_size(ui->eye_r, bw, bh);
  lv_obj_set_size(ui->glow_l, gw, gh);
  lv_obj_set_size(ui->glow_r, gw, gh);
  lv_obj_set_style_radius(ui->eye_l, bw / 2, LV_PART_MAIN);
  lv_obj_set_style_radius(ui->eye_r, bw / 2, LV_PART_MAIN);
  lv_obj_set_style_radius(ui->glow_l, gw / 2, LV_PART_MAIN);
  lv_obj_set_style_radius(ui->glow_r, gw / 2, LV_PART_MAIN);

  float gx = m->gazeX * (vw * 0.12f);
  float gy = m->gazeY * (vh * 0.10f);

  lv_obj_set_align(ui->eye_l, LV_ALIGN_CENTER);
  lv_obj_set_align(ui->eye_r, LV_ALIGN_CENTER);
  lv_obj_set_align(ui->glow_l, LV_ALIGN_CENTER);
  lv_obj_set_align(ui->glow_r, LV_ALIGN_CENTER);
  lv_obj_set_x(ui->eye_l, (-gap) + (int32_t)gx);
  lv_obj_set_x(ui->eye_r, gap + (int32_t)gx);
  lv_obj_set_y(ui->eye_l, (int32_t)gy);
  lv_obj_set_y(ui->eye_r, (int32_t)gy);
  lv_obj_set_x(ui->glow_l, (-gap) + (int32_t)gx);
  lv_obj_set_x(ui->glow_r, gap + (int32_t)gx);
  lv_obj_set_y(ui->glow_l, (int32_t)gy);
  lv_obj_set_y(ui->glow_r, (int32_t)gy);

  lv_opa_t glow = (lv_opa_t)constrain((int)m->glowOpa, 0, 255);
  lv_obj_set_style_bg_opa(ui->glow_l, (lv_opa_t)(glow * 40 / 255), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->glow_r, (lv_opa_t)(glow * 40 / 255), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(ui->eye_l, (lv_opa_t)(glow / 3), LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(ui->eye_r, (lv_opa_t)(glow / 3), LV_PART_MAIN);

  lv_opa_t scan = (lv_opa_t)constrain((int)m->scanOpa, 0, 255);
  lv_obj_set_style_bg_opa(ui->scan_bar, scan, LV_PART_MAIN);

  int32_t lidPx = (int32_t)(bh * m->lid);
  if (lidPx > bh) {
    lidPx = bh;
  }
  lv_obj_set_height(ui->lid_l, lidPx);
  lv_obj_set_height(ui->lid_r, lidPx);

  int32_t tilt = (int32_t)(m->tiltDeg * 10);
  lv_obj_set_style_transform_rotation(ui->visor, tilt, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_x(ui->visor, vw / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(ui->visor, vh / 2, LV_PART_MAIN);
}

#endif /* EVE_ENABLE_EYES */
