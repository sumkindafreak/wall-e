#include "config.h"

#if EVE_ENABLE_EYES

#include "eve_eye_display_driver.h"
#include <esp_heap_caps.h>
#include <stdlib.h>

#if EVE_FACE_GFX_READY
#include <Arduino_GFX_Library.h>
static Arduino_DataBus* s_busLeft = nullptr;
static Arduino_DataBus* s_busRight = nullptr;
static Arduino_GFX* s_gfxLeft = nullptr;
static Arduino_GFX* s_gfxRight = nullptr;
static int8_t eveGfxRstPin(void) {
  return (EVE_TFT_RST >= 0) ? (int8_t)EVE_TFT_RST : GFX_NOT_DEFINED;
}
#endif

static lv_display_t* s_disp[EVE_EYE_PANEL_COUNT];
static lv_color_t* s_buf[EVE_EYE_PANEL_COUNT];
static bool s_dual = false;

bool eveEyeDisplayIsDualPhysical(void) {
  return s_dual;
}

lv_display_t* eveEyeDisplayGet(EveEyePanelId panel) {
  if (panel >= EVE_EYE_PANEL_COUNT) {
    return nullptr;
  }
  return s_disp[panel];
}

static void flush_to_gfx(Arduino_GFX* gfx, lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
#if EVE_FACE_GFX_READY
  if (gfx) {
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)px_map, (int16_t)w, (int16_t)h);
  }
#else
  (void)gfx;
  (void)area;
  (void)px_map;
#endif
  lv_display_flush_ready(disp);
}

static void flush_left(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
#if EVE_FACE_GFX_READY
  flush_to_gfx(s_gfxLeft, disp, area, px_map);
#else
  (void)disp;
  (void)area;
  (void)px_map;
  lv_display_flush_ready(disp);
#endif
}

static void flush_right(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
#if EVE_FACE_GFX_READY
  flush_to_gfx(s_gfxRight, disp, area, px_map);
#else
  (void)disp;
  (void)area;
  (void)px_map;
  lv_display_flush_ready(disp);
#endif
}

static lv_display_t* make_display(EveEyePanelId panel, void (*flush_cb)(lv_display_t*, const lv_area_t*, uint8_t*)) {
  lv_display_t* disp = lv_display_create(EVE_FACE_LCD_HOR_RES, EVE_FACE_LCD_VER_RES);
  if (!disp) {
    return nullptr;
  }
  lv_display_set_flush_cb(disp, flush_cb);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

  size_t buf_bytes = (size_t)EVE_FACE_LCD_HOR_RES * (size_t)EVE_FACE_LVGL_BUF_LINES * sizeof(lv_color_t);
  lv_color_t* buf = (lv_color_t*)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buf) {
    buf = (lv_color_t*)malloc(buf_bytes);
  }
  if (!buf) {
    lv_display_delete(disp);
    return nullptr;
  }
  s_buf[panel] = buf;
  lv_display_set_buffers(disp, buf, nullptr, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  return disp;
}

void eveEyeDisplayInit(void) {
  s_dual = EVE_FACE_DUAL_PHYSICAL;

#if EVE_FACE_GFX_READY
  s_busLeft = new Arduino_ESP32SPI(EVE_TFT_DC, EVE_LEFT_EYE_CS, EVE_TFT_SPI_SCK, EVE_TFT_SPI_MOSI,
                                   EVE_TFT_SPI_MISO);
  s_gfxLeft = new Arduino_ST7789(s_busLeft, eveGfxRstPin(), 0, true, (int16_t)EVE_FACE_LCD_HOR_RES,
                                 (int16_t)EVE_FACE_LCD_VER_RES, 0, 0);
  if (s_gfxLeft->begin()) {
    s_gfxLeft->fillScreen(BLACK);
  }

  if (s_dual) {
    s_busRight = new Arduino_ESP32SPI(EVE_TFT_DC, EVE_RIGHT_EYE_CS, EVE_TFT_SPI_SCK, EVE_TFT_SPI_MOSI,
                                      EVE_TFT_SPI_MISO);
    s_gfxRight = new Arduino_ST7789(s_busRight, eveGfxRstPin(), 0, true, (int16_t)EVE_FACE_LCD_HOR_RES,
                                    (int16_t)EVE_FACE_LCD_VER_RES, 0, 0);
    if (s_gfxRight->begin()) {
      s_gfxRight->fillScreen(BLACK);
    }
    Serial.println(F("[EVE_EYE] Dual physical panels (left + right CS)"));
  } else {
    Serial.println(F("[EVE_EYE] Single panel legacy path (left CS only)"));
  }
#endif

  s_disp[EVE_EYE_PANEL_LEFT] = make_display(EVE_EYE_PANEL_LEFT, flush_left);
  if (s_dual) {
    s_disp[EVE_EYE_PANEL_RIGHT] = make_display(EVE_EYE_PANEL_RIGHT, flush_right);
  }
}

void eveEyeDisplayTickLvgl(uint32_t dtMs) {
  lv_tick_inc(dtMs);
  lv_timer_handler();
}

#endif /* EVE_ENABLE_EYES */
