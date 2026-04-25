#pragma once

#include <lvgl.h>

// WALL-E / industrial dark theme palette.
#define LVGL_WALLE_BG        lv_color_hex(0x050505)
#define LVGL_WALLE_PANEL     lv_color_hex(0x111111)
#define LVGL_WALLE_PANEL_2   lv_color_hex(0x1A1A1A)
#define LVGL_WALLE_BORDER    lv_color_hex(0x353535)
/** High-contrast text on dark panels (CYD readability). */
#define LVGL_WALLE_TEXT      lv_color_white()
#define LVGL_WALLE_TEXT_DIM  lv_color_hex(0xD8D8D8)
#define LVGL_WALLE_ACCENT    lv_color_hex(0xFFC20E)
#define LVGL_WALLE_OK        lv_color_hex(0x52D38A)
#define LVGL_WALLE_WARN      lv_color_hex(0xFFD200)
#define LVGL_WALLE_ERR       lv_color_hex(0xE3484D)

typedef struct {
  lv_style_t screen;
  lv_style_t panel;
  lv_style_t card;
  lv_style_t button;
  lv_style_t button_stop;
  lv_style_t button_primary;
  lv_style_t title;
  lv_style_t text;
  lv_style_t text_dim;
} LvglWalleStyles;

void lvglStyleInit(LvglWalleStyles* s);
void lvglStyleDeinit(LvglWalleStyles* s);
