#include "lvgl_style.h"

void lvglStyleInit(LvglWalleStyles* s) {
  if (!s) return;

  lv_style_init(&s->screen);
  lv_style_set_bg_color(&s->screen, LVGL_WALLE_BG);
  lv_style_set_bg_opa(&s->screen, LV_OPA_COVER);
  lv_style_set_text_color(&s->screen, lv_color_white());

  lv_style_init(&s->panel);
  lv_style_set_bg_color(&s->panel, LVGL_WALLE_PANEL);
  lv_style_set_bg_opa(&s->panel, LV_OPA_COVER);
  lv_style_set_border_width(&s->panel, 1);
  lv_style_set_border_color(&s->panel, LVGL_WALLE_BORDER);
  lv_style_set_radius(&s->panel, 10);
  lv_style_set_pad_all(&s->panel, 8);

  lv_style_init(&s->card);
  lv_style_set_bg_color(&s->card, LVGL_WALLE_PANEL_2);
  lv_style_set_bg_opa(&s->card, LV_OPA_COVER);
  lv_style_set_border_width(&s->card, 1);
  lv_style_set_border_color(&s->card, LVGL_WALLE_BORDER);
  lv_style_set_radius(&s->card, 8);
  lv_style_set_pad_all(&s->card, 8);

  lv_style_init(&s->button);
  lv_style_set_radius(&s->button, 10);
  lv_style_set_bg_color(&s->button, LVGL_WALLE_PANEL_2);
  lv_style_set_bg_opa(&s->button, LV_OPA_COVER);
  lv_style_set_border_width(&s->button, 2);
  lv_style_set_border_color(&s->button, lv_color_hex(0x5A4A00));
  lv_style_set_text_color(&s->button, lv_color_white());
  lv_style_set_pad_ver(&s->button, 10);
  lv_style_set_pad_hor(&s->button, 12);
  lv_style_set_shadow_width(&s->button, 8);
  lv_style_set_shadow_color(&s->button, lv_color_hex(0x2B2400));
  lv_style_set_shadow_opa(&s->button, LV_OPA_40);

  lv_style_init(&s->button_stop);
  lv_style_set_radius(&s->button_stop, 10);
  lv_style_set_bg_color(&s->button_stop, LVGL_WALLE_ERR);
  lv_style_set_bg_opa(&s->button_stop, LV_OPA_COVER);
  lv_style_set_text_color(&s->button_stop, lv_color_white());
  lv_style_set_border_width(&s->button_stop, 0);

  lv_style_init(&s->button_primary);
  lv_style_set_radius(&s->button_primary, 10);
  lv_style_set_bg_color(&s->button_primary, LVGL_WALLE_ACCENT);
  lv_style_set_bg_opa(&s->button_primary, LV_OPA_COVER);
  lv_style_set_text_color(&s->button_primary, lv_color_black());
  lv_style_set_border_width(&s->button_primary, 0);
  lv_style_set_shadow_width(&s->button_primary, 14);
  lv_style_set_shadow_color(&s->button_primary, lv_color_hex(0x7A6400));
  lv_style_set_shadow_opa(&s->button_primary, LV_OPA_60);

  lv_style_init(&s->title);
  lv_style_set_text_color(&s->title, lv_color_white());
  lv_style_set_text_font(&s->title, LV_FONT_DEFAULT);

  lv_style_init(&s->text);
  lv_style_set_text_color(&s->text, lv_color_white());

  lv_style_init(&s->text_dim);
  lv_style_set_text_color(&s->text_dim, LVGL_WALLE_TEXT_DIM);
}

void lvglStyleDeinit(LvglWalleStyles* s) {
  if (!s) return;
  lv_style_reset(&s->screen);
  lv_style_reset(&s->panel);
  lv_style_reset(&s->card);
  lv_style_reset(&s->button);
  lv_style_reset(&s->button_stop);
  lv_style_reset(&s->button_primary);
  lv_style_reset(&s->title);
  lv_style_reset(&s->text);
  lv_style_reset(&s->text_dim);
}
