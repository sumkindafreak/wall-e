#include "lvgl_input.h"

#include "../../wall_e_master_controller/ads1115_input.h"
#include "../../wall_e_master_controller/sx1509_input.h"
#include <math.h>

#if __has_include(<XPT2046_Bitbang_Slim.h>)
#include <XPT2046_Bitbang_Slim.h>
static XPT2046_Bitbang_Slim s_touch(25, 39, 32, 33);
#define LVGL_USE_BITBANG_SLIM 1
#else
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
static SPIClass s_touchSpi(HSPI);
static XPT2046_Touchscreen s_touch(33, 36);
#define LVGL_USE_BITBANG_SLIM 0
#endif

static bool s_touchPressed = false;
static int16_t s_touchX = 0;
static int16_t s_touchY = 0;

static bool s_joystickEnabled = true;
static bool s_uiDriveEnabled = false;
static int8_t s_uiLeft = 0;
static int8_t s_uiRight = 0;

static float s_vj1x = 0.0f, s_vj1y = 0.0f;
static float s_vj2x = 0.0f, s_vj2y = 0.0f;
static bool s_vj1Active = false;
static bool s_vj2Active = false;

static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}
static bool s_estopEdge = false;
static bool s_prevBoth = false;

static inline int16_t mapTouchX(int32_t x) {
  long v = map((long)x, 200, 3700, 0, 319);
  if (v < 0) v = 0;
  if (v > 319) v = 319;
  return (int16_t)v;
}
static inline int16_t mapTouchY(int32_t y) {
  long v = map((long)y, 240, 3800, 0, 239);
  if (v < 0) v = 0;
  if (v > 239) v = 239;
  return (int16_t)v;
}

void lvglInputInit(void) {
#if LVGL_USE_BITBANG_SLIM
  s_touch.begin();
#else
  s_touchSpi.begin(25, 39, 32, 33);
  s_touch.begin(s_touchSpi);
  s_touch.setRotation(1);
#endif
}

void lvglInputRead(lv_indev_t* indev, lv_indev_data_t* data) {
  LV_UNUSED(indev);

#if LVGL_USE_BITBANG_SLIM
  int16_t rx = 0, ry = 0;
  s_touchPressed = s_touch.touched();
  if (s_touchPressed) {
    s_touch.readData(&rx, &ry);
    s_touchX = mapTouchX(rx);
    s_touchY = mapTouchY(ry);
  }
#else
  s_touchPressed = s_touch.touched();
  if (s_touchPressed) {
    TS_Point p = s_touch.getPoint();
    s_touchX = mapTouchX(p.x);
    s_touchY = mapTouchY(p.y);
  }
#endif

  data->state = s_touchPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  data->point.x = s_touchX;
  data->point.y = s_touchY;
}

void lvglInputSetUiDrive(int8_t left, int8_t right) {
  s_uiLeft = left;
  s_uiRight = right;
  s_uiDriveEnabled = (left != 0 || right != 0);
}

void lvglInputSetUiDriveEnabled(bool en) { s_uiDriveEnabled = en; }

void lvglInputSetVirtualJoy1(float x, float y, bool active) {
  s_vj1x = clampf(x, -1.0f, 1.0f);
  s_vj1y = clampf(y, -1.0f, 1.0f);
  s_vj1Active = active;
}

void lvglInputSetVirtualJoy2(float x, float y, bool active) {
  s_vj2x = clampf(x, -1.0f, 1.0f);
  s_vj2y = clampf(y, -1.0f, 1.0f);
  s_vj2Active = active;
}

bool lvglInputVirtualJoy1Active(void) { return s_vj1Active; }
bool lvglInputVirtualJoy2Active(void) { return s_vj2Active; }
float lvglInputVirtualJoy1X(void) { return s_vj1x; }
float lvglInputVirtualJoy1Y(void) { return s_vj1y; }
float lvglInputVirtualJoy2X(void) { return s_vj2x; }
float lvglInputVirtualJoy2Y(void) { return s_vj2y; }

void lvglInputUpdateDriveFromHardware(DriveState* out) {
  if (!out) return;
  out->leftSpeed = 0;
  out->rightSpeed = 0;
  out->precisionMode = false;

  const bool bothHeld = isBothJoystickButtonsHeld();
  if (bothHeld && !s_prevBoth) s_estopEdge = true;
  s_prevBoth = bothHeld;

  if (!s_joystickEnabled) {
    if (s_vj2Active) {
      const float throttle = -s_vj2y;
      const float turn = s_vj2x;
      float left = throttle + turn;
      float right = throttle - turn;
      left = clampf(left, -1.0f, 1.0f);
      right = clampf(right, -1.0f, 1.0f);
      out->leftSpeed = (int8_t)(left * 100.0f);
      out->rightSpeed = (int8_t)(right * 100.0f);
    } else if (s_uiDriveEnabled) {
      out->leftSpeed = s_uiLeft;
      out->rightSpeed = s_uiRight;
    }
    if (!isDeadmanButtonHeld()) {
      out->leftSpeed = 0;
      out->rightSpeed = 0;
    }
    return;
  }

  const JoystickState& js = getJoystickState();
  const bool joyActive = js.active[JOY2_X] || js.active[JOY2_Y];
  if (joyActive) {
    joystickToDriveState(out);
  } else if (s_vj2Active) {
    /* Same tank mix as ads1115_input::joystickToDriveState (Joy2). */
    const float throttle = -s_vj2y;
    const float turn = s_vj2x;
    float left = throttle + turn;
    float right = throttle - turn;
    left = clampf(left, -1.0f, 1.0f);
    right = clampf(right, -1.0f, 1.0f);
    out->leftSpeed = (int8_t)(left * 100.0f);
    out->rightSpeed = (int8_t)(right * 100.0f);
    out->precisionMode = false;
  } else if (s_uiDriveEnabled) {
    out->leftSpeed = s_uiLeft;
    out->rightSpeed = s_uiRight;
  }

  if (!isDeadmanButtonHeld()) {
    out->leftSpeed = 0;
    out->rightSpeed = 0;
  }
}

bool lvglInputConsumeEstopEdge(void) {
  const bool e = s_estopEdge;
  s_estopEdge = false;
  return e;
}

void lvglInputSetJoystickEnabled(bool en) { s_joystickEnabled = en; }
bool lvglInputJoystickEnabled(void) { return s_joystickEnabled; }
