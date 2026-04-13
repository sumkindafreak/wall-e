#include "config.h"
#include "eve_face_display.h"

#if EVE_ENABLE_EYES

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "eve_expression_state.h"
#include "eve_eye_animations.h"
#include "eve_eye_renderer.h"

#if EVE_FACE_GFX_READY
#include <Arduino_GFX_Library.h>
static Arduino_DataBus* s_bus = nullptr;
static Arduino_GFX* s_gfx = nullptr;
static int8_t eveGfxRstPin(void) {
  return (EVE_TFT_RST >= 0) ? (int8_t)EVE_TFT_RST : GFX_NOT_DEFINED;
}
#endif

static lv_display_t* s_disp = nullptr;
static lv_color_t* s_buf1 = nullptr;
static uint32_t s_lastMs = 0;
static EveEyeUi s_ui;
static EveEyeTarget s_smooth;
static bool s_smoothInit = false;
static int s_lastExpr = -1;

static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
#if EVE_FACE_GFX_READY
  if (s_gfx) {
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);
    s_gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)px_map, (int16_t)w, (int16_t)h);
  }
#else
  (void)area;
  (void)px_map;
#endif
  lv_display_flush_ready(disp);
}

static void lerpTarget(EveEyeTarget* s, const EveEyeTarget* t, float a) {
#define L(F) s->F += (t->F - s->F) * a
  L(eyeSep);
  L(eyeScaleX);
  L(eyeScaleY);
  L(gazeX);
  L(gazeY);
  L(glowOpa);
  L(scanOpa);
  L(tiltDeg);
  L(squint);
#undef L
}

void eveFaceDisplayInit(void) {
#if EVE_FACE_GFX_READY
  s_bus = new Arduino_ESP32SPI(EVE_TFT_DC, EVE_TFT_LEFT_CS, EVE_TFT_SPI_SCK, EVE_TFT_SPI_MOSI,
                               EVE_TFT_SPI_MISO);
  s_gfx = new Arduino_ST7789(s_bus, eveGfxRstPin(), 0, true, (int16_t)EVE_FACE_LCD_HOR_RES,
                             (int16_t)EVE_FACE_LCD_VER_RES, 0, 0);
  if (!s_gfx->begin()) {
    Serial.println(F("[EVE_FACE] GFX begin failed — check pins"));
  } else {
    s_gfx->fillScreen(BLACK);
  }
#endif

  lv_init();
  s_disp = lv_display_create(EVE_FACE_LCD_HOR_RES, EVE_FACE_LCD_VER_RES);
  lv_display_set_flush_cb(s_disp, flush_cb);
  lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);

  size_t buf_bytes = (size_t)EVE_FACE_LCD_HOR_RES * (size_t)EVE_FACE_LVGL_BUF_LINES * sizeof(lv_color_t);
  s_buf1 = (lv_color_t*)heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!s_buf1) {
    s_buf1 = (lv_color_t*)malloc(buf_bytes);
  }
  lv_display_set_buffers(s_disp, s_buf1, nullptr, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_obj_t* scr = lv_screen_active();
  eveEyeRendererInit(scr, &s_ui);
  eveEyeAnimationsInit(&s_ui);
  eveExpressionInit();

  s_lastMs = millis();
  Serial.println(F("[EVE_FACE] LVGL face initialized"));
}

void eveFaceDisplayTick(uint32_t nowMs) {
  if (!s_disp) {
    return;
  }

  uint32_t dt = nowMs - s_lastMs;
  if (s_lastMs == 0 || dt > 64) {
    dt = 64;
  }
  s_lastMs = nowMs;
  lv_tick_inc((uint32_t)dt);
  float dts = dt * 0.001f;

#if EVE_FACE_DEBUG_BENCH
  eveExpressionDebugSerialPoll();
#endif

  eveExpressionTick(nowMs);
  eveEyeAnimationsTick(nowMs, dts);

  EveEyeTarget tgt;
  eveExpressionGetTarget(&tgt);
  if (!s_smoothInit) {
    s_smooth = tgt;
    s_smoothInit = true;
  }
  lerpTarget(&s_smooth, &tgt, 0.16f);

  float blink = eveEyeAnimationsBlinkLid();
  float lidGoal = fminf(1.f, fmaxf(tgt.lid, blink));
  s_smooth.lid += (lidGoal - s_smooth.lid) * 0.42f;

  float mgx = 0.f;
  float mgy = 0.f;
  eveEyeAnimationsGetMicroGaze(&mgx, &mgy);
  s_smooth.gazeX += (tgt.gazeX + mgx * 0.55f - s_smooth.gazeX) * 0.22f;
  s_smooth.gazeY += (tgt.gazeY + mgy * 0.55f - s_smooth.gazeY) * 0.22f;

  float pulse = 5.f * sinf((float)nowMs * 0.0028f);
  s_smooth.glowOpa = fminf(255.f, fmaxf(0.f, s_smooth.glowOpa + pulse * 0.04f));

  eveEyeRendererApply(&s_ui, &s_smooth);

  int cur = (int)eveExpressionGetCurrent();
  if (cur != s_lastExpr) {
    s_lastExpr = cur;
    Serial.print(F("[EVE_FACE] Expression -> "));
#if EVE_FACE_DEBUG_BENCH
    Serial.println(eveExpressionName((EveExpressionId)cur));
#else
    Serial.println(cur);
#endif
    if (cur == (int)EVE_EXPR_LOW_BATTERY) {
      Serial.println(F("[EVE_FACE] Low battery visual state active"));
    }
  }

#if EVE_FACE_DEBUG_BENCH
  char line[40];
  snprintf(line, sizeof(line), "%s", eveExpressionName((EveExpressionId)cur));
  eveEyeRendererSetDebugText(&s_ui, line);
#endif

  lv_timer_handler();
}

#else /* !EVE_ENABLE_EYES */

void eveFaceDisplayInit(void) {}
void eveFaceDisplayTick(uint32_t) {}

#endif /* EVE_ENABLE_EYES */
