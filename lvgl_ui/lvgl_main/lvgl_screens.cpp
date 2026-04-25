#include "lvgl_screens.h"
#include "lvgl_style.h"
#include "lvgl_ui.h"   // for lvglUiIsSdReady / lvglUi* actions
#include "lvgl_input.h"
#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <SD.h>
#include "../../wall_e_master_controller/sd_browser.h"
#include "../../wall_e_master_controller/sd_manager.h"

static LvglWalleStyles s_style;
static lv_obj_t* s_root = nullptr;
static lv_obj_t* s_tabs = nullptr;

static lv_obj_t* s_lblBattery = nullptr;
static lv_obj_t* s_lblMode = nullptr;
static lv_obj_t* s_lblLink = nullptr;
static lv_obj_t* s_barLeft = nullptr;
static lv_obj_t* s_barRight = nullptr;
static lv_obj_t* s_lblJoy = nullptr;
static lv_obj_t* s_lblDriveLink = nullptr;
static lv_obj_t* s_lblMeterL = nullptr;
static lv_obj_t* s_lblMeterR = nullptr;
static lv_obj_t* s_padHead = nullptr;
static lv_obj_t* s_padDrive = nullptr;
static lv_obj_t* s_joy1Dot = nullptr;
static lv_obj_t* s_joy2Dot = nullptr;
static lv_obj_t* s_taLogs = nullptr;
static lv_obj_t* s_lblSensors = nullptr;
static lv_obj_t* s_sdList = nullptr;
static bool s_sdDirty = true;  // kept for now; SD tab will be passive

static uint32_t s_logStamp = 0;

static lv_obj_t* s_eveHead = nullptr;
static lv_obj_t* s_eveArm = nullptr;
static lv_obj_t* s_eveStatus = nullptr;
static const LvglRuntimeData* s_eventData = nullptr;

/* AUDIO soundboard */
static lv_obj_t* s_audioScroll = nullptr;
static lv_obj_t* s_audioStatus = nullptr;
static bool s_audioListDirty = true;

#ifndef LVGL_AUDIO_MAX_FILES
#define LVGL_AUDIO_MAX_FILES 24
#endif
#ifndef LVGL_AUDIO_FN_LEN
#define LVGL_AUDIO_FN_LEN 40
#endif
static char s_audioFiles[LVGL_AUDIO_MAX_FILES][LVGL_AUDIO_FN_LEN];
static uint8_t s_audioFileCount = 0;

static int cmpAudioName(const void* a, const void* b) {
  return strcmp((const char*)a, (const char*)b);
}

static void scanDirAudio(const char* path) {
  File d = SD.open(path);
  if (!d || !d.isDirectory()) {
    if (d) d.close();
    return;
  }
  for (uint32_t scan = 0;; ++scan) {
    File f = d.openNextFile();
    if (!f) break;
    if (!f.isDirectory()) {
      String nm = f.name();
      const int slash = nm.lastIndexOf('/');
      if (slash >= 0) nm = nm.substring(slash + 1);
      nm.toLowerCase();
      if (nm.endsWith(".mp3") || nm.endsWith(".wav")) {
        if (s_audioFileCount < LVGL_AUDIO_MAX_FILES) {
          nm.toCharArray(s_audioFiles[s_audioFileCount], LVGL_AUDIO_FN_LEN - 1);
          s_audioFileCount++;
        }
      }
    }
    f.close();
    if ((scan & 7u) == 7u) yield();
  }
  d.close();
}

static void soundboardScanSd(void) {
  s_audioFileCount = 0;
  memset(s_audioFiles, 0, sizeof(s_audioFiles));
  if (!lvglUiIsSdReady() || !sdIsAvailable()) return;

  scanDirAudio("/wall_e/audio");
  if (s_audioFileCount == 0) scanDirAudio("/audio");
  if (s_audioFileCount > 1) {
    qsort(s_audioFiles, s_audioFileCount, LVGL_AUDIO_FN_LEN, cmpAudioName);
  }
}

static void cb_audio_track(lv_event_t* e) {
  uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
  if (!s_eventData || !s_eventData->linkOk) {
    lvglUiShowToast("No base link", 1000);
    return;
  }
  const uint8_t track = (uint8_t)(idx + 1u);
  if (track == 0) return;
  lvglUiActionPlayAudioTrack(track);
}

static void cb_audio_refresh(lv_event_t* e) {
  LV_UNUSED(e);
  s_audioListDirty = true;
}

static void soundboardRebuildList(void) {
  if (!s_audioScroll) return;
  lv_obj_clean(s_audioScroll);
  soundboardScanSd();

  if (s_audioStatus) {
    if (!lvglUiIsSdReady() || !sdIsAvailable()) {
      lv_label_set_text(s_audioStatus, "SD: not available — put .mp3 in /wall_e/audio");
    } else if (s_audioFileCount == 0) {
      lv_label_set_text(s_audioStatus,
                          "No clips in /wall_e/audio\n(Base link: use track # = row order)");
    } else {
      char st[96];
      snprintf(st, sizeof(st), "SD: %u files — tap sends track #1..%u (ESP-NOW)",
               (unsigned)s_audioFileCount, (unsigned)s_audioFileCount);
      lv_label_set_text(s_audioStatus, st);
    }
  }

  for (uint8_t i = 0; i < s_audioFileCount; i++) {
    if (i != 0 && (i & 3u) == 0u) yield();
    lv_obj_t* row = lv_obj_create(s_audioScroll);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 30);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 6, 0);
    char num[6];
    snprintf(num, sizeof(num), "%u", (unsigned)(i + 1));
    lv_obj_t* ln = lv_label_create(row);
    lv_label_set_text(ln, num);
    lv_obj_set_style_text_color(ln, LVGL_WALLE_ACCENT, 0);
    lv_obj_set_width(ln, 22);
    lv_obj_t* b = lv_button_create(row);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_height(b, 26);
    lv_obj_add_style(b, &s_style.button, 0);
    lv_obj_t* t = lv_label_create(b);
    lv_label_set_text(t, s_audioFiles[i]);
    lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
    lv_obj_set_width(t, lv_pct(100));
    lv_obj_clear_flag(t, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(b, cb_audio_track, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
  }
}

static lv_obj_t* make_btn(lv_obj_t* parent, const char* txt, lv_event_cb_t cb, void* user, bool primary, bool stop) {
  lv_obj_t* b = lv_button_create(parent);
  lv_obj_add_style(b, &s_style.button, 0);
  if (primary) lv_obj_add_style(b, &s_style.button_primary, 0);
  if (stop) lv_obj_add_style(b, &s_style.button_stop, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  return b;
}

static void styleMeterBar(lv_obj_t* bar) {
  lv_obj_set_style_bg_color(bar, LVGL_WALLE_PANEL_2, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(bar, LVGL_WALLE_BORDER, LV_PART_MAIN);
  lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(bar, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, LVGL_WALLE_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_bar_set_mode(bar, LV_BAR_MODE_SYMMETRICAL);
  lv_bar_set_value(bar, 0, LV_ANIM_OFF);
}

static void styleSlider(lv_obj_t* slider) {
  lv_obj_set_style_bg_color(slider, LVGL_WALLE_PANEL_2, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, LVGL_WALLE_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, LVGL_WALLE_ACCENT, LV_PART_KNOB);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
  lv_obj_set_style_border_color(slider, lv_color_black(), LV_PART_KNOB);
  lv_obj_set_style_border_width(slider, 1, LV_PART_KNOB);
}

/** LVGL 9 tab bar: row of buttons with flex_grow(1) squeezes labels; make bar scroll horizontally. */
static void apply_scrollable_tab_bar(lv_obj_t* tabview) {
  lv_obj_t* bar = lv_tabview_get_tab_bar(tabview);
  if (!bar) return;

  lv_obj_add_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(bar, LV_DIR_HOR);
  lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_scroll_snap_x(bar, LV_SCROLL_SNAP_NONE);
  lv_obj_set_style_pad_column(bar, 4, 0);
  lv_obj_set_style_pad_hor(bar, 2, 0);

  const uint32_t n = lv_tabview_get_tab_count(tabview);
  for (uint32_t i = 0; i < n; i++) {
    lv_obj_t* btn = lv_tabview_get_tab_button(tabview, (int32_t)i);
    if (!btn) continue;
    lv_obj_set_style_flex_grow(btn, 0, 0);
    lv_obj_set_width(btn, LV_SIZE_CONTENT);
    lv_obj_set_height(btn, lv_pct(100));
    lv_obj_set_style_min_width(btn, 48, 0);
    lv_obj_set_style_pad_hor(btn, 8, 0);
  }
  lv_obj_update_layout(tabview);
}

static void cb_nav(lv_event_t* e) {
  uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
  lvglScreensSetPage((LvglNavPage)idx);
}

static void cb_drive_press(lv_event_t* e) {
  uintptr_t code = (uintptr_t)lv_event_get_user_data(e);
  switch (code) {
    case 0: lvglUiActionSetUiDrive(80, 80); break;     // F
    case 1: lvglUiActionSetUiDrive(-80, -80); break;   // B
    case 2: lvglUiActionSetUiDrive(-70, 70); break;    // L
    case 3: lvglUiActionSetUiDrive(70, -70); break;    // R
    default: break;
  }
}

static void cb_drive_release(lv_event_t* e) {
  LV_UNUSED(e);
  lvglUiActionSetUiDrive(0, 0);
}

/** Hold-to-drive directional pad (PRESSED / PRESSING, release on let-go). */
static lv_obj_t* make_dir_btn(lv_obj_t* parent, const char* txt, uintptr_t code, bool primary) {
  lv_obj_t* b = lv_button_create(parent);
  lv_obj_add_style(b, &s_style.button, 0);
  if (primary) lv_obj_add_style(b, &s_style.button_primary, 0);
  lv_obj_add_event_cb(b, cb_drive_press, LV_EVENT_PRESSED, (void*)code);
  lv_obj_add_event_cb(b, cb_drive_press, LV_EVENT_PRESSING, (void*)code);
  lv_obj_add_event_cb(b, cb_drive_release, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(b, cb_drive_release, LV_EVENT_PRESS_LOST, nullptr);
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  return b;
}

static void cb_stop(lv_event_t* e) {
  LV_UNUSED(e);
  lvglUiActionStopAll();
}

static void cb_anim(lv_event_t* e) {
  uintptr_t animId = (uintptr_t)lv_event_get_user_data(e);
  lvglUiActionTriggerAnimation((uint8_t)animId);
}

static void place_joy_dot(lv_obj_t* pad, lv_obj_t* dot, float nx, float ny) {
  if (!pad || !dot) return;
  const lv_coord_t w = lv_obj_get_width(pad);
  const lv_coord_t h = lv_obj_get_height(pad);
  if (w < 12 || h < 12) return;
  const lv_coord_t knob = lv_obj_get_width(dot);
  const float R = (float)((w < h ? w : h) / 2 - 6 - knob / 2);
  if (R < 4.0f) return;
  const lv_coord_t cx = w / 2 - knob / 2;
  const lv_coord_t cy = h / 2 - knob / 2;
  const lv_coord_t dx = (lv_coord_t)(nx * R);
  const lv_coord_t dy = (lv_coord_t)(ny * R);
  lv_obj_set_pos(dot, cx + dx, cy + dy);
}

static void updateJoystickVisuals(const JoystickState* js) {
  if (!s_joy1Dot || !s_joy2Dot || !s_padHead || !s_padDrive) return;

  float j1x = 0.0f, j1y = 0.0f, j2x = 0.0f, j2y = 0.0f;
  if (lvglInputVirtualJoy1Active()) {
    j1x = lvglInputVirtualJoy1X();
    j1y = lvglInputVirtualJoy1Y();
  } else if (js) {
    j1x = js->processed[JOY1_X];
    j1y = js->processed[JOY1_Y];
  }
  if (lvglInputVirtualJoy2Active()) {
    j2x = lvglInputVirtualJoy2X();
    j2y = lvglInputVirtualJoy2Y();
  } else if (js) {
    j2x = js->processed[JOY2_X];
    j2y = js->processed[JOY2_Y];
  }

  /* Display mirror: invert horizontal so screen matches robot L/R */
  place_joy_dot(s_padHead, s_joy1Dot, -j1x, j1y);
  place_joy_dot(s_padDrive, s_joy2Dot, -j2x, j2y);
}

static void cb_joypad_square(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
  lv_obj_t* pad = (lv_obj_t*)lv_event_get_target(e);
  const lv_coord_t w = lv_obj_get_width(pad);
  if (w > 4) {
    lv_obj_set_height(pad, w);
  }
}

static void virt_joy_norm_from_screen(lv_obj_t* pad, int32_t px, int32_t py, float* outNx, float* outNy) {
  lv_area_t a;
  lv_obj_get_coords(pad, &a);
  const lv_coord_t w = lv_obj_get_width(pad);
  const lv_coord_t h = lv_obj_get_height(pad);
  const int32_t cx = (int32_t)a.x1 + (int32_t)w / 2;
  const int32_t cy = (int32_t)a.y1 + (int32_t)h / 2;
  const float R = (float)((w < h ? w : h) / 2 - 8);
  if (R < 6.0f) {
    *outNx = 0.0f;
    *outNy = 0.0f;
    return;
  }
  float x = (float)(px - cx) / R;
  float y = (float)(py - cy) / R;
  const float mag = sqrtf(x * x + y * y);
  if (mag > 1.0f && mag > 1e-4f) {
    x /= mag;
    y /= mag;
  }
  *outNx = x;
  *outNy = y;
}

static void cb_virt_joy(lv_event_t* e) {
  const lv_event_code_t code = lv_event_get_code(e);
  const uintptr_t kind = (uintptr_t)lv_event_get_user_data(e);

  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (kind == 0) {
      lvglInputSetVirtualJoy1(0.0f, 0.0f, false);
    } else {
      lvglInputSetVirtualJoy2(0.0f, 0.0f, false);
      lvglUiActionSetUiDrive(0, 0);
    }
    return;
  }
  if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) return;

  lv_indev_t* indev = lv_indev_get_act();
  if (!indev) return;
  lv_point_t p;
  lv_indev_get_point(indev, &p);
  lv_obj_t* pad = (lv_obj_t*)lv_event_get_current_target(e);
  float nx = 0.0f;
  float ny = 0.0f;
  virt_joy_norm_from_screen(pad, p.x, p.y, &nx, &ny);

  if (kind == 0) {
    lvglInputSetVirtualJoy1(nx, ny, true);
  } else {
    lvglInputSetVirtualJoy2(nx, ny, true);
  }
}

static void make_joy_stick_column(lv_obj_t* joyRow, const char* title, const char* subtitle, uintptr_t kind) {
  lv_obj_t* col = lv_obj_create(joyRow);
  lv_obj_remove_style_all(col);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow(col, 1);
  lv_obj_set_width(col, lv_pct(48));
  lv_obj_set_style_pad_gap(col, 4, 0);

  lv_obj_t* lab = lv_label_create(col);
  lv_label_set_text(lab, title);
  lv_obj_set_style_text_color(lab, lv_color_white(), 0);
  lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(lab, lv_pct(100));
  if (subtitle && subtitle[0]) {
    lv_obj_t* sub = lv_label_create(col);
    lv_label_set_text(sub, subtitle);
    lv_obj_add_style(sub, &s_style.text_dim, 0);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(sub, lv_pct(100));
  }

  lv_obj_t* pad = lv_obj_create(col);
  lv_obj_remove_style_all(pad);
  lv_obj_set_width(pad, lv_pct(100));
  lv_obj_set_style_min_height(pad, 104, 0);
  lv_obj_set_flex_grow(pad, 1);
  lv_obj_add_flag(pad, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(pad, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(pad, LV_OPA_30, 0);
  lv_obj_set_style_bg_color(pad, LVGL_WALLE_PANEL_2, 0);
  lv_obj_set_style_border_width(pad, 2, 0);
  lv_obj_set_style_border_color(pad, (kind == 0) ? lv_color_hex(0x8C7B55) : LVGL_WALLE_ACCENT, 0);
  lv_obj_add_event_cb(pad, cb_joypad_square, LV_EVENT_SIZE_CHANGED, nullptr);
  lv_obj_add_event_cb(pad, cb_virt_joy, LV_EVENT_PRESSED, (void*)kind);
  lv_obj_add_event_cb(pad, cb_virt_joy, LV_EVENT_PRESSING, (void*)kind);
  lv_obj_add_event_cb(pad, cb_virt_joy, LV_EVENT_RELEASED, (void*)kind);
  lv_obj_add_event_cb(pad, cb_virt_joy, LV_EVENT_PRESS_LOST, (void*)kind);

  lv_obj_t* dot = lv_obj_create(pad);
  lv_obj_remove_style_all(dot);
  lv_obj_set_size(dot, 16, 16);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dot, (kind == 0) ? lv_color_hex(0xB89A4A) : LVGL_WALLE_ACCENT, 0);
  lv_obj_set_style_border_width(dot, 1, 0);
  lv_obj_set_style_border_color(dot, lv_color_white(), 0);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);

  if (kind == 0) {
    s_padHead = pad;
    s_joy1Dot = dot;
  } else {
    s_padDrive = pad;
    s_joy2Dot = dot;
  }
}

static void cb_slider_servo(lv_event_t* e) {
  lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
  uintptr_t id = (uintptr_t)lv_event_get_user_data(e);
  int16_t v = (int16_t)lv_slider_get_value(slider);
  if (id == 0) lvglUiActionSetHeadTiltPct(v);
  else if (id == 1) lvglUiActionSetHeadPanPct(v);
  else if (id == 2) lvglUiActionSetEyebrowPct(v);
  else if (id == 3) lvglUiActionSetServoSpeedPct(v);
}

static void cb_slider_volume(lv_event_t* e) {
  lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
  lvglUiActionSetVolumePct((int16_t)lv_slider_get_value(slider));
}

static void cb_profile(lv_event_t* e) {
  lvglUiActionProfileSet((uint8_t)(uintptr_t)lv_event_get_user_data(e));
}

static void cb_brightness(lv_event_t* e) {
  lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
  int16_t v = (int16_t)lv_slider_get_value(slider);
  lvglUiActionBrightnessSet((uint8_t)((v * 255) / 100));
}

static void cb_joy_toggle(lv_event_t* e) {
  lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
  bool en = lv_obj_has_state(sw, LV_STATE_CHECKED);
  lvglUiActionToggleJoystick(en);
}

static void cb_sd_refresh(lv_event_t* e) {
  LV_UNUSED(e);
  lvglUiActionSdRefresh();
  s_sdDirty = true;
}

static void cb_sd_up(lv_event_t* e) {
  LV_UNUSED(e);
  lvglUiActionSdUp();
  s_sdDirty = true;
}

static void cb_sd_item(lv_event_t* e) {
  uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
  lvglUiActionSdOpenSelected((uint16_t)idx);
  s_sdDirty = true;
}

static void cb_save_pose(lv_event_t* e) {
  LV_UNUSED(e);
  lvglUiActionSavePosition();
}

static void cb_load_pose(lv_event_t* e) {
  LV_UNUSED(e);
  lvglUiActionLoadPosition();
}

static void cb_eve_send(lv_event_t* e) {
  LV_UNUSED(e);
  if (!s_eventData || !s_eveHead || !s_eveArm) return;
  if (!s_eventData->eveUartOk) {
    lvglUiShowToast("EVE not connected", 1200);
    return;
  }
  int head = (int)lv_slider_get_value(s_eveHead);
  int arm = (int)lv_slider_get_value(s_eveArm);
  lvglUiActionEveSendServo((int16_t)head, (int16_t)arm);
  lvglUiShowToast("Pose sent to Base", 1000);
}

static void cb_profile_editor(lv_event_t* e) {
  LV_UNUSED(e);
  lvglUiProfileEditorOpen();
}

static lv_obj_t* build_home(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 8, 0);
  lv_obj_set_style_pad_row(tab, 8, 0);

  s_lblBattery = lv_label_create(tab);
  lv_label_set_text(s_lblBattery, "Battery: --.-V");
  lv_obj_set_style_text_color(s_lblBattery, lv_color_white(), 0);
  s_lblMode = lv_label_create(tab);
  lv_label_set_text(s_lblMode, "Mode: MANUAL");
  lv_obj_set_style_text_color(s_lblMode, lv_color_white(), 0);
  s_lblLink = lv_label_create(tab);
  lv_label_set_text(s_lblLink, "Link: OFFLINE");
  lv_obj_set_style_text_color(s_lblLink, lv_color_white(), 0);

  lv_obj_t* row = lv_obj_create(tab);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, lv_pct(100), 100);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_gap(row, 8, 0);

  lv_obj_t* b1 = make_btn(row, "DRIVE", cb_nav, (void*)LVGL_NAV_DRIVE, true, false);
  lv_obj_t* b2 = make_btn(row, "BEHAV", cb_nav, (void*)LVGL_NAV_BEHAVIOR, false, false);
  lv_obj_t* b3 = make_btn(row, "SETTINGS", cb_nav, (void*)LVGL_NAV_SETTINGS, false, false);
  lv_obj_t* b4 = make_btn(row, "SYSTEM", cb_nav, (void*)LVGL_NAV_SYSTEM, false, false);
  lv_obj_set_size(b1, 145, 44);
  lv_obj_set_size(b2, 145, 44);
  lv_obj_set_size(b3, 145, 44);
  lv_obj_set_size(b4, 145, 44);
  return tab;
}

static lv_obj_t* build_drive(lv_obj_t* tab) {
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 6, 0);
  lv_obj_set_style_pad_row(tab, 6, 0);

  lv_obj_t* headRow = lv_obj_create(tab);
  lv_obj_remove_style_all(headRow);
  lv_obj_set_width(headRow, lv_pct(100));
  lv_obj_set_height(headRow, 22);
  lv_obj_set_flex_flow(headRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(headRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* title = lv_label_create(headRow);
  lv_label_set_text(title, "Drive");
  lv_obj_add_style(title, &s_style.title, 0);
  s_lblDriveLink = lv_label_create(headRow);
  lv_label_set_text(s_lblDriveLink, "OFFLINE");
  lv_obj_set_style_text_color(s_lblDriveLink, LVGL_WALLE_ERR, 0);

  lv_obj_t* meterCard = lv_obj_create(tab);
  lv_obj_add_style(meterCard, &s_style.card, 0);
  lv_obj_set_width(meterCard, lv_pct(100));
  lv_obj_set_height(meterCard, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(meterCard, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(meterCard, 4, 0);
  lv_obj_set_style_pad_all(meterCard, 8, 0);

  lv_obj_t* meterTitle = lv_label_create(meterCard);
  lv_label_set_text(meterTitle, "Treads (commanded)");
  lv_obj_add_style(meterTitle, &s_style.text_dim, 0);

  lv_obj_t* meterBars = lv_obj_create(meterCard);
  lv_obj_remove_style_all(meterBars);
  lv_obj_set_width(meterBars, lv_pct(100));
  lv_obj_set_height(meterBars, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(meterBars, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(meterBars, 10, 0);

  lv_obj_t* colL = lv_obj_create(meterBars);
  lv_obj_remove_style_all(colL);
  lv_obj_set_flex_flow(colL, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow(colL, 1);
  lv_obj_set_width(colL, lv_pct(48));
  lv_obj_set_style_pad_gap(colL, 2, 0);
  lv_obj_t* labL = lv_label_create(colL);
  lv_label_set_text(labL, "Right motor");
  lv_obj_add_style(labL, &s_style.text_dim, 0);
  s_barLeft = lv_bar_create(colL);
  lv_obj_set_width(s_barLeft, lv_pct(100));
  lv_obj_set_height(s_barLeft, 14);
  lv_bar_set_range(s_barLeft, -100, 100);
  styleMeterBar(s_barLeft);
  s_lblMeterL = lv_label_create(colL);
  lv_label_set_text(s_lblMeterL, "0");
  lv_obj_set_style_text_color(s_lblMeterL, LVGL_WALLE_ACCENT, 0);
  lv_obj_set_style_text_align(s_lblMeterL, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t* colR = lv_obj_create(meterBars);
  lv_obj_remove_style_all(colR);
  lv_obj_set_flex_flow(colR, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow(colR, 1);
  lv_obj_set_width(colR, lv_pct(48));
  lv_obj_set_style_pad_gap(colR, 2, 0);
  lv_obj_t* labR = lv_label_create(colR);
  lv_label_set_text(labR, "Left motor");
  lv_obj_add_style(labR, &s_style.text_dim, 0);
  s_barRight = lv_bar_create(colR);
  lv_obj_set_width(s_barRight, lv_pct(100));
  lv_obj_set_height(s_barRight, 14);
  lv_bar_set_range(s_barRight, -100, 100);
  styleMeterBar(s_barRight);
  s_lblMeterR = lv_label_create(colR);
  lv_label_set_text(s_lblMeterR, "0");
  lv_obj_set_style_text_color(s_lblMeterR, LVGL_WALLE_ACCENT, 0);
  lv_obj_set_style_text_align(s_lblMeterR, LV_TEXT_ALIGN_CENTER, 0);

  s_lblJoy = lv_label_create(tab);
  lv_label_set_text(s_lblJoy, "Head 0.00 / 0.00\nDrive 0.00 / 0.00");
  lv_label_set_long_mode(s_lblJoy, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(s_lblJoy, lv_color_white(), 0);
  lv_obj_set_style_text_line_space(s_lblJoy, 2, 0);
  lv_obj_set_width(s_lblJoy, lv_pct(100));

  lv_obj_t* joyRow = lv_obj_create(tab);
  lv_obj_remove_style_all(joyRow);
  lv_obj_set_width(joyRow, lv_pct(100));
  lv_obj_set_flex_flow(joyRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(joyRow, 8, 0);
  lv_obj_set_flex_grow(joyRow, 1);
  lv_obj_set_style_min_height(joyRow, 112, 0);
  make_joy_stick_column(joyRow, "HEAD", "Pan / tilt", 0);
  make_joy_stick_column(joyRow, "DRIVE", "Steer / throttle", 1);

  lv_obj_t* favRow = lv_obj_create(tab);
  lv_obj_remove_style_all(favRow);
  lv_obj_set_width(favRow, lv_pct(100));
  lv_obj_set_height(favRow, 68);
  lv_obj_set_flex_flow(favRow, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_gap(favRow, 6, 0);
  lv_obj_set_flex_align(favRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  for (uint8_t i = 0; i < 6; i++) {
    char t[6];
    snprintf(t, sizeof(t), "F%u", (unsigned)(i + 1));
    lv_obj_t* b = make_btn(favRow, t, cb_anim, (void*)(uintptr_t)i, false, false);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_style_min_width(b, 96, 0);
    lv_obj_set_height(b, 30);
  }

  lv_obj_t* dirRow = lv_obj_create(tab);
  lv_obj_remove_style_all(dirRow);
  lv_obj_set_width(dirRow, lv_pct(100));
  lv_obj_set_height(dirRow, 32);
  lv_obj_set_flex_flow(dirRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(dirRow, 6, 0);
  lv_obj_t* bf = make_dir_btn(dirRow, "F", 0, true);
  lv_obj_set_flex_grow(bf, 1);
  lv_obj_set_height(bf, 30);
  lv_obj_t* bb = make_dir_btn(dirRow, "B", 1, false);
  lv_obj_set_flex_grow(bb, 1);
  lv_obj_set_height(bb, 30);
  lv_obj_t* bl = make_dir_btn(dirRow, "L", 2, false);
  lv_obj_set_flex_grow(bl, 1);
  lv_obj_set_height(bl, 30);
  lv_obj_t* br = make_dir_btn(dirRow, "R", 3, false);
  lv_obj_set_flex_grow(br, 1);
  lv_obj_set_height(br, 30);

  lv_obj_t* bot = lv_obj_create(tab);
  lv_obj_remove_style_all(bot);
  lv_obj_set_width(bot, lv_pct(100));
  lv_obj_set_height(bot, 36);
  lv_obj_set_flex_flow(bot, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(bot, 4, 0);

  lv_obj_t* dock = make_btn(bot, "Dock", cb_anim, (void*)(uintptr_t)9, true, false);
  lv_obj_set_flex_grow(dock, 0);
  lv_obj_set_width(dock, 52);
  lv_obj_set_height(dock, 34);
  lv_obj_t* cancel = make_btn(bot, "Cancel", cb_drive_release, nullptr, false, false);
  lv_obj_set_width(cancel, 52);
  lv_obj_set_height(cancel, 34);
  lv_obj_t* stop = make_btn(bot, "E-STOP", cb_stop, nullptr, false, true);
  lv_obj_set_width(stop, 72);
  lv_obj_set_height(stop, 34);
  lv_obj_t* n1 = make_btn(bot, "Sys", cb_nav, (void*)LVGL_NAV_SYSTEM, false, false);
  lv_obj_set_flex_grow(n1, 1);
  lv_obj_set_height(n1, 34);
  lv_obj_t* n2 = make_btn(bot, "Beh", cb_nav, (void*)LVGL_NAV_BEHAVIOR, false, false);
  lv_obj_set_flex_grow(n2, 1);
  lv_obj_set_height(n2, 34);
  lv_obj_t* n3 = make_btn(bot, "Prf", cb_nav, (void*)LVGL_NAV_SETTINGS, false, false);
  lv_obj_set_flex_grow(n3, 1);
  lv_obj_set_height(n3, 34);
  lv_obj_t* n4 = make_btn(bot, "Aud", cb_nav, (void*)LVGL_NAV_AUDIO, false, false);
  lv_obj_set_flex_grow(n4, 1);
  lv_obj_set_height(n4, 34);
  return tab;
}

static lv_obj_t* build_behavior(lv_obj_t* tab) {
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 6, 0);
  lv_obj_set_style_pad_row(tab, 6, 0);

  const char* names[] = {"Head Tilt", "Head Rotate", "Eyebrows", "Servo Speed"};
  for (uint8_t i = 0; i < 4; i++) {
    lv_obj_t* row = lv_obj_create(tab);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 30);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 6, 0);
    lv_obj_t* l = lv_label_create(row);
    lv_label_set_text(l, names[i]);
    lv_obj_set_style_text_color(l, lv_color_white(), 0);
    lv_obj_set_width(l, 96);
    lv_obj_t* s = lv_slider_create(row);
    lv_obj_set_flex_grow(s, 1);
    lv_obj_set_height(s, 14);
    lv_slider_set_range(s, 0, 100);
    lv_slider_set_value(s, 50, LV_ANIM_OFF);
    styleSlider(s);
    lv_obj_add_event_cb(s, cb_slider_servo, LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)i);
  }

  lv_obj_t* pillRow = lv_obj_create(tab);
  lv_obj_remove_style_all(pillRow);
  lv_obj_set_width(pillRow, lv_pct(100));
  lv_obj_set_height(pillRow, 32);
  lv_obj_set_flex_flow(pillRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(pillRow, 8, 0);
  lv_obj_t* sv = make_btn(pillRow, "Save", cb_save_pose, nullptr, false, false);
  lv_obj_t* ld = make_btn(pillRow, "Load", cb_load_pose, nullptr, true, false);
  lv_obj_set_flex_grow(sv, 1);
  lv_obj_set_height(sv, 28);
  lv_obj_set_flex_grow(ld, 1);
  lv_obj_set_height(ld, 28);

  lv_obj_t* hint = lv_label_create(tab);
  lv_label_set_text(hint, "More screens: swipe the tab bar at the top");
  lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(hint, lv_pct(100));
  lv_obj_set_style_text_color(hint, LVGL_WALLE_TEXT_DIM, 0);

  lv_obj_t* grid = lv_obj_create(tab);
  lv_obj_remove_style_all(grid);
  lv_obj_set_width(grid, lv_pct(100));
  lv_obj_set_flex_grow(grid, 1);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_gap(grid, 6, 0);
  lv_obj_set_style_pad_top(grid, 4, 0);
  for (uint8_t i = 0; i < 6; i++) {
    char t[8];
    snprintf(t, sizeof(t), "A%u", (unsigned)i);
    lv_obj_t* b = make_btn(grid, t, cb_anim, (void*)(uintptr_t)i, false, false);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_style_min_width(b, 92, 0);
    lv_obj_set_height(b, 36);
  }
  return tab;
}

static lv_obj_t* build_audio(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 6, 0);
  lv_obj_set_style_pad_row(tab, 4, 0);

  s_audioStatus = lv_label_create(tab);
  lv_label_set_text(s_audioStatus, "Loading…");
  lv_label_set_long_mode(s_audioStatus, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(s_audioStatus, lv_pct(100));
  lv_obj_set_style_text_color(s_audioStatus, LVGL_WALLE_TEXT_DIM, 0);

  lv_obj_t* top = lv_obj_create(tab);
  lv_obj_remove_style_all(top);
  lv_obj_set_width(top, lv_pct(100));
  lv_obj_set_height(top, 30);
  lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(top, 6, 0);
  lv_obj_t* bRef = make_btn(top, "Refresh list", cb_audio_refresh, nullptr, true, false);
  lv_obj_set_flex_grow(bRef, 1);
  lv_obj_set_height(bRef, 28);

  s_audioScroll = lv_obj_create(tab);
  lv_obj_set_width(s_audioScroll, lv_pct(100));
  lv_obj_set_flex_grow(s_audioScroll, 1);
  lv_obj_set_flex_flow(s_audioScroll, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(s_audioScroll, 4, 0);
  lv_obj_set_style_pad_all(s_audioScroll, 4, 0);
  lv_obj_set_style_bg_opa(s_audioScroll, LV_OPA_TRANSP, 0);
  lv_obj_set_scrollbar_mode(s_audioScroll, LV_SCROLLBAR_MODE_AUTO);

  lv_obj_t* l = lv_label_create(tab);
  lv_label_set_text(l, "UI volume (beeps)");
  lv_obj_set_style_text_color(l, lv_color_white(), 0);
  lv_obj_t* s = lv_slider_create(tab);
  lv_obj_set_size(s, lv_pct(100), 18);
  lv_slider_set_range(s, 0, 100);
  lv_slider_set_value(s, 70, LV_ANIM_OFF);
  styleSlider(s);
  lv_obj_add_event_cb(s, cb_slider_volume, LV_EVENT_VALUE_CHANGED, nullptr);

  s_audioListDirty = true;
  return tab;
}

static lv_obj_t* build_settings(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 8, 0);
  lv_obj_set_style_pad_row(tab, 6, 0);

  lv_obj_t* row = lv_obj_create(tab);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, lv_pct(100), 54);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row, 8, 0);
  lv_obj_t* p0 = make_btn(row, "Kid", cb_profile, (void*)0, false, false);
  lv_obj_t* p1 = make_btn(row, "Demo", cb_profile, (void*)1, false, false);
  lv_obj_t* p2 = make_btn(row, "Adv", cb_profile, (void*)2, false, false);
  lv_obj_set_size(p0, 90, 46);
  lv_obj_set_size(p1, 90, 46);
  lv_obj_set_size(p2, 90, 46);

  lv_obj_t* lb = lv_label_create(tab);
  lv_label_set_text(lb, "Brightness");
  lv_obj_set_style_text_color(lb, lv_color_white(), 0);
  lv_obj_t* sb = lv_slider_create(tab);
  lv_obj_set_size(sb, lv_pct(100), 18);
  lv_slider_set_range(sb, 10, 100);
  lv_slider_set_value(sb, 85, LV_ANIM_OFF);
  styleSlider(sb);
  lv_obj_add_event_cb(sb, cb_brightness, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* row2 = lv_obj_create(tab);
  lv_obj_remove_style_all(row2);
  lv_obj_set_size(row2, lv_pct(100), 36);
  lv_obj_set_flex_flow(row2, LV_FLEX_FLOW_ROW);
  lv_obj_t* t = lv_label_create(row2);
  lv_label_set_text(t, "Joystick enabled");
  lv_obj_set_style_text_color(t, lv_color_white(), 0);
  lv_obj_t* sw = lv_switch_create(row2);
  lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(sw, LVGL_WALLE_PANEL_2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(sw, LVGL_WALLE_BORDER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sw, LVGL_WALLE_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(sw, LVGL_WALLE_TEXT, LV_PART_KNOB);
  lv_obj_add_event_cb(sw, cb_joy_toggle, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* edit = make_btn(tab, "Edit profile tuning…", cb_profile_editor, nullptr, true, false);
  lv_obj_set_width(edit, lv_pct(100));
  lv_obj_set_height(edit, 36);
  return tab;
}

static lv_obj_t* build_sd(lv_obj_t* tab) {
  // SAFE PLACEHOLDER: do NOT drive sdBrowser from LVGL clone.
  // The full SD explorer remains on the original CYD UI.
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 8, 0);
  lv_obj_set_style_pad_row(tab, 6, 0);

  lv_obj_t* title = lv_label_create(tab);
  lv_label_set_text(title, "SD tools (main UI)");
  lv_obj_set_style_text_color(title, LVGL_WALLE_TEXT, 0);

  lv_obj_t* msg = lv_label_create(tab);
  lv_label_set_text(msg, "Use original CYD screen\nfor full SD explorer.");
  lv_label_set_long_mode(msg, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(msg, lv_pct(100));
  lv_obj_set_style_text_color(msg, lv_color_white(), 0);

  // No LVGL-driven SD list or buttons here to avoid crashes.
  s_sdList = nullptr;
  s_sdDirty = false;
  return tab;
}

static lv_obj_t* build_eve(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 8, 0);
  lv_obj_set_style_pad_row(tab, 6, 0);

  lv_obj_t* title = lv_label_create(tab);
  lv_label_set_text(title, "EVE remote");
  lv_obj_set_style_text_color(title, LVGL_WALLE_TEXT, 0);

  s_eveStatus = lv_label_create(tab);
  lv_label_set_text(s_eveStatus, "UART: --");
  lv_obj_set_style_text_color(s_eveStatus, lv_color_white(), 0);

  lv_obj_t* lh = lv_label_create(tab);
  lv_label_set_text(lh, "Head pan (deg)");
  lv_obj_set_style_text_color(lh, lv_color_white(), 0);
  s_eveHead = lv_slider_create(tab);
  lv_obj_set_size(s_eveHead, lv_pct(100), 18);
  lv_slider_set_range(s_eveHead, 45, 135);
  lv_slider_set_value(s_eveHead, 90, LV_ANIM_OFF);
  styleSlider(s_eveHead);

  lv_obj_t* la = lv_label_create(tab);
  lv_label_set_text(la, "Right arm (deg)");
  lv_obj_set_style_text_color(la, lv_color_white(), 0);
  s_eveArm = lv_slider_create(tab);
  lv_obj_set_size(s_eveArm, lv_pct(100), 18);
  lv_slider_set_range(s_eveArm, 0, 180);
  lv_slider_set_value(s_eveArm, 90, LV_ANIM_OFF);
  styleSlider(s_eveArm);

  lv_obj_t* row = lv_obj_create(tab);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, lv_pct(100), 40);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row, 8, 0);
  lv_obj_t* bSend = make_btn(row, "Send pose", cb_eve_send, nullptr, true, false);
  lv_obj_t* bHome = make_btn(row, "Home", cb_nav, (void*)LVGL_NAV_HOME, false, false);
  lv_obj_set_size(bSend, 168, 36);
  lv_obj_set_size(bHome, 100, 36);
  return tab;
}

static lv_obj_t* build_system(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 8, 0);
  lv_obj_set_style_pad_row(tab, 6, 0);
  s_lblSensors = lv_label_create(tab);
  lv_label_set_text(s_lblSensors, "V: --  I: --  Link: --");
  lv_obj_set_style_text_color(s_lblSensors, lv_color_white(), 0);
  s_taLogs = lv_textarea_create(tab);
  lv_obj_set_size(s_taLogs, lv_pct(100), 160);
  lv_textarea_set_placeholder_text(s_taLogs, "Runtime logs...");
  lv_textarea_set_one_line(s_taLogs, false);
  lv_obj_clear_flag(s_taLogs, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(s_taLogs, LVGL_WALLE_PANEL_2, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_taLogs, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(s_taLogs, LVGL_WALLE_BORDER, LV_PART_MAIN);
  lv_obj_set_style_text_color(s_taLogs, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_color(s_taLogs, lv_color_hex(0xBEBEBE), LV_PART_TEXTAREA_PLACEHOLDER);
  return tab;
}

void lvglScreensInit(void) {
  lvglStyleInit(&s_style);

  s_root = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(s_root);
  lv_obj_add_style(s_root, &s_style.screen, 0);
  lv_obj_set_size(s_root, lv_pct(100), lv_pct(100));

  s_tabs = lv_tabview_create(s_root);
  lv_obj_set_size(s_tabs, lv_pct(100), lv_pct(100));
  /* Compact bar; tab buttons scroll horizontally when needed */
  lv_tabview_set_tab_bar_size(s_tabs, 28);
  lv_obj_set_style_bg_color(s_tabs, LVGL_WALLE_BG, 0);
  lv_obj_set_style_bg_opa(s_tabs, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(s_tabs, lv_color_white(), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(s_tabs, LVGL_WALLE_PANEL, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(s_tabs, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_border_color(s_tabs, LVGL_WALLE_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_border_width(s_tabs, 1, LV_PART_ITEMS);
  lv_obj_set_style_pad_left(s_tabs, 0, LV_PART_ITEMS);
  lv_obj_set_style_pad_right(s_tabs, 0, LV_PART_ITEMS);
  lv_obj_set_style_pad_top(s_tabs, 2, LV_PART_ITEMS);
  lv_obj_set_style_pad_bottom(s_tabs, 2, LV_PART_ITEMS);
  lv_obj_set_style_text_color(s_tabs, lv_color_black(), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(s_tabs, LVGL_WALLE_ACCENT, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_opa(s_tabs, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_shadow_width(s_tabs, 6, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_shadow_color(s_tabs, lv_color_hex(0x7A6400), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_shadow_opa(s_tabs, LV_OPA_40, LV_PART_ITEMS | LV_STATE_CHECKED);

  build_home(lv_tabview_add_tab(s_tabs, "HOME"));
  build_drive(lv_tabview_add_tab(s_tabs, "DRIVE"));
  build_behavior(lv_tabview_add_tab(s_tabs, "BEHAV"));
  build_audio(lv_tabview_add_tab(s_tabs, "AUDIO"));
  build_settings(lv_tabview_add_tab(s_tabs, "SET"));
  build_sd(lv_tabview_add_tab(s_tabs, "SD"));
  build_system(lv_tabview_add_tab(s_tabs, "SYS"));
  build_eve(lv_tabview_add_tab(s_tabs, "EVE"));

  apply_scrollable_tab_bar(s_tabs);
}

void lvglScreensSetPage(LvglNavPage page) {
  if (!s_tabs) return;
  const uint32_t idx = (uint32_t)page;
  lv_tabview_set_active(s_tabs, idx, LV_ANIM_ON);
  lv_obj_t* btn = lv_tabview_get_tab_button(s_tabs, (int32_t)idx);
  if (btn) {
    lv_obj_scroll_to_view(btn, LV_ANIM_ON);
  }
}

void lvglScreensTick(const LvglRuntimeData* data) {
  if (!data) return;
  s_eventData = data;

  if (s_tabs && lv_tabview_get_tab_active(s_tabs) != (uint32_t)LVGL_NAV_DRIVE) {
    lvglInputSetVirtualJoy1(0.0f, 0.0f, false);
    lvglInputSetVirtualJoy2(0.0f, 0.0f, false);
  }

  if (s_lblBattery) {
    char b[48];
    snprintf(b, sizeof(b), "Battery: %.2fV | %.2fA", data->batteryV, data->currentA);
    lv_label_set_text(s_lblBattery, b);
  }
  if (s_lblMode) {
    const char* mode = "MANUAL";
    if (data->mode == 1) mode = "AUTO";
    else if (data->mode == 2) mode = "SAFE";
    char m[24];
    snprintf(m, sizeof(m), "Mode: %s", mode);
    lv_label_set_text(s_lblMode, m);
  }
  if (s_lblLink) {
    lv_label_set_text(s_lblLink, data->linkOk ? "Link: CONNECTED" : "Link: OFFLINE");
  }
  if (s_lblDriveLink) {
    lv_label_set_text(s_lblDriveLink, data->linkOk ? "LINK OK" : "OFFLINE");
    lv_obj_set_style_text_color(s_lblDriveLink, data->linkOk ? LVGL_WALLE_OK : LVGL_WALLE_ERR, 0);
  }

  /* Mirror L/R: left widget shows right motor, right widget shows left motor */
  if (s_barLeft) lv_bar_set_value(s_barLeft, data->drive.rightSpeed, LV_ANIM_OFF);
  if (s_barRight) lv_bar_set_value(s_barRight, data->drive.leftSpeed, LV_ANIM_OFF);
  if (s_lblMeterL) {
    char ml[8];
    snprintf(ml, sizeof(ml), "%d", data->drive.rightSpeed);
    lv_label_set_text(s_lblMeterL, ml);
  }
  if (s_lblMeterR) {
    char mr[8];
    snprintf(mr, sizeof(mr), "%d", data->drive.leftSpeed);
    lv_label_set_text(s_lblMeterR, mr);
  }
  if (s_lblJoy) {
    float h1x, h1y, h2x, h2y;
    if (lvglInputVirtualJoy1Active()) {
      h1x = lvglInputVirtualJoy1X();
      h1y = lvglInputVirtualJoy1Y();
    } else {
      h1x = data->joystick.processed[JOY1_X];
      h1y = data->joystick.processed[JOY1_Y];
    }
    if (lvglInputVirtualJoy2Active()) {
      h2x = lvglInputVirtualJoy2X();
      h2y = lvglInputVirtualJoy2Y();
    } else {
      h2x = data->joystick.processed[JOY2_X];
      h2y = data->joystick.processed[JOY2_Y];
    }
    char j[96];
    snprintf(j, sizeof(j), "Head  %.2f / %.2f\nDrive %.2f / %.2f", -h1x, h1y, -h2x, h2y);
    lv_label_set_text(s_lblJoy, j);
  }
  updateJoystickVisuals(&data->joystick);

  if (s_lblSensors) {
    char s[80];
    snprintf(s, sizeof(s), "V: %.2f | I: %.2f | Link: %s", data->batteryV, data->currentA, data->linkOk ? "OK" : "NO");
    lv_label_set_text(s_lblSensors, s);
  }

  if (s_eveStatus) {
    lv_label_set_text(s_eveStatus, data->eveUartOk ? "UART: connected" : "UART: offline");
  }

  if (s_taLogs) {
    uint32_t now = millis();
    if ((now - s_logStamp) > 1000) {
      s_logStamp = now;
      char line[96];
      snprintf(line, sizeof(line), "[%lu] L:%d R:%d\n", (unsigned long)now, data->drive.leftSpeed, data->drive.rightSpeed);
      lv_textarea_add_text(s_taLogs, line);
    }
  }

  if (s_tabs && lv_tabview_get_tab_active(s_tabs) == (uint32_t)LVGL_NAV_AUDIO && s_audioScroll) {
    if (s_audioListDirty) {
      soundboardRebuildList();
      s_audioListDirty = false;
    }
  }

  // SD tab for LVGL clone is a passive placeholder; original UI owns full SD explorer.
}
