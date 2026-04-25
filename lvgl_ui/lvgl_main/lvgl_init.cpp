#include "lvgl_init.h"
#include "lvgl_input.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

// USER-CUSTOMIZABLE: panel colour / endian correction (CYD ILI9341 + LVGL v9 + TFT_eSPI)
//
// 1) Wrong hues / R-B swapped on SPI: try toggling LVGL_TFT_SWAP_RGB565_BYTES (0 vs 1).
//    Or set LVGL_DISPLAY_USE_RGB565_SWAPPED to 1 and set LVGL_TFT_SWAP_RGB565_BYTES to 0
//    (uses LVGL's native LV_COLOR_FORMAT_RGB565_SWAPPED — no extra flush work).
//
// 2) Whole screen looks inverted / negative: set LVGL_TFT_INVERT_DISPLAY to 1.
//
// 3) Some setups need TFT_eSPI byte swap instead of LVGL buffer swap: set
//    LVGL_TFT_ESPI_SWAP_BYTES to 1 and LVGL_TFT_SWAP_RGB565_BYTES to 0.
#ifndef LVGL_DISPLAY_USE_RGB565_SWAPPED
#define LVGL_DISPLAY_USE_RGB565_SWAPPED 0
#endif
#ifndef LVGL_TFT_SWAP_RGB565_BYTES
#define LVGL_TFT_SWAP_RGB565_BYTES 1
#endif
#ifndef LVGL_TFT_INVERT_DISPLAY
#define LVGL_TFT_INVERT_DISPLAY 0
#endif
#ifndef LVGL_TFT_ESPI_SWAP_BYTES
#define LVGL_TFT_ESPI_SWAP_BYTES 0
#endif
#if LVGL_TFT_SWAP_RGB565_BYTES && LVGL_TFT_ESPI_SWAP_BYTES
#error Use either LVGL_TFT_SWAP_RGB565_BYTES or LVGL_TFT_ESPI_SWAP_BYTES, not both
#endif
#if LVGL_DISPLAY_USE_RGB565_SWAPPED && LVGL_TFT_SWAP_RGB565_BYTES
#error With LVGL_DISPLAY_USE_RGB565_SWAPPED set LVGL_TFT_SWAP_RGB565_BYTES to 0
#endif
#if LVGL_DISPLAY_USE_RGB565_SWAPPED && LVGL_TFT_ESPI_SWAP_BYTES
#error With LVGL_DISPLAY_USE_RGB565_SWAPPED set LVGL_TFT_ESPI_SWAP_BYTES to 0
#endif

static TFT_eSPI s_tft = TFT_eSPI();
static lv_display_t* s_disp = nullptr;
static lv_indev_t* s_indev = nullptr;

// Partial buffers for smoother rendering on ESP32.
static lv_color_t* s_buf1 = nullptr;
static lv_color_t* s_buf2 = nullptr;

static void swapRgb565BytesInPlace(uint16_t* p, uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    uint16_t c = p[i];
    p[i] = (uint16_t)((c << 8) | (c >> 8));
  }
}

static void lvglFlush(lv_display_t* display, const lv_area_t* area, uint8_t* pxMap) {
  LV_UNUSED(display);
  const int32_t w = area->x2 - area->x1 + 1;
  const int32_t h = area->y2 - area->y1 + 1;
  const uint32_t px = (uint32_t)(w * h);
  uint16_t* pixels = (uint16_t*)pxMap;

#if !LVGL_DISPLAY_USE_RGB565_SWAPPED && LVGL_TFT_SWAP_RGB565_BYTES
  swapRgb565BytesInPlace(pixels, px);
#endif

  s_tft.startWrite();
  s_tft.setAddrWindow(area->x1, area->y1, w, h);
  s_tft.pushPixels(pixels, px);
  s_tft.endWrite();

#if !LVGL_DISPLAY_USE_RGB565_SWAPPED && LVGL_TFT_SWAP_RGB565_BYTES
  swapRgb565BytesInPlace(pixels, px);
#endif

  lv_display_flush_ready(display);
}

void lvglPlatformInit(void) {
  lv_init();

  s_tft.init();
  s_tft.setRotation(1);  // 320x240 landscape (CYD)
#if LVGL_TFT_INVERT_DISPLAY
  s_tft.invertDisplay(true);
#else
  s_tft.invertDisplay(false);
#endif
#if LVGL_TFT_ESPI_SWAP_BYTES
  s_tft.setSwapBytes(true);
#endif
  s_tft.fillScreen(TFT_BLACK);

  const uint32_t bufPx = 320u * 24u;  // 24 lines
  s_buf1 = (lv_color_t*)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  s_buf2 = (lv_color_t*)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!s_buf1 || !s_buf2) {
    // Fallback to plain heap if DMA/internal allocation fails.
    if (!s_buf1) s_buf1 = (lv_color_t*)malloc(bufPx * sizeof(lv_color_t));
    if (!s_buf2) s_buf2 = (lv_color_t*)malloc(bufPx * sizeof(lv_color_t));
  }

  s_disp = lv_display_create(320, 240);
#if LVGL_DISPLAY_USE_RGB565_SWAPPED
  lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
#else
  lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
#endif
  lv_display_set_flush_cb(s_disp, lvglFlush);
  lv_display_set_buffers(s_disp, s_buf1, s_buf2, bufPx * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lvglInputInit();
  s_indev = lv_indev_create();
  lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(s_indev, lvglInputRead);
}

void lvglPlatformTask(void) {
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  /* lastMs==0: first run after setup — delta was (now - 0) and could be seconds, which
   * makes lv_tick_inc + lv_timer_handler run a huge catch-up and blocks touch/input. */
  if (lastMs == 0) {
    lastMs = now;
    lv_tick_inc(1);
  } else {
    uint32_t delta = now - lastMs;
    lastMs = now;
    if (delta == 0) delta = 1;
    /* Cap for long loop stalls (SPI, WiFi, etc.); unbounded inc also starves indev. */
    if (delta > 100u) delta = 100u;
    lv_tick_inc(delta);
  }
  yield();
  lv_timer_handler();
  yield();
}
