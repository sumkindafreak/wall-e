#include "lvgl_ui.h"
#include "lvgl_screens.h"
#include "lvgl_input.h"
#include "lvgl_style.h"

#include "../../wall_e_master_controller/packet_control.h"
#include "../../wall_e_master_controller/motion_engine.h"
#include "../../wall_e_master_controller/profiles.h"
#include "../../wall_e_master_controller/audio_system.h"
#include "../../wall_e_master_controller/sd_browser.h"
#include "../../wall_e_master_controller/espnow_control.h"
#include "../../wall_e_master_controller/ads1115_input.h"
#include "../../firmware_common/include/walle_link_packet.h"

static LvglRuntimeData s_data = {};
static bool s_hasTelemetry = false;
static bool s_eveUartPrev = false;

// SD ready flag from main (guards SD calls)
static bool s_sdReady = false;

// --- ADDED: Toast state ---
static lv_obj_t* s_toast = nullptr;
static lv_obj_t* s_toastLabel = nullptr;
static uint32_t s_toastUntilMs = 0;
static bool s_toastVisible = false;

// --- ADDED: Keyboard modal state ---
static lv_obj_t* s_kbModal = nullptr;
static lv_obj_t* s_kbTextarea = nullptr;
static lv_obj_t* s_keyboard = nullptr;
static LvglKeyboardCallback s_kbCallback = nullptr;
static void* s_kbUserData = nullptr;

// Forward decls
static void toastEnsureCreated(void);
static void toastUpdate(uint32_t nowMs);
static void kbEnsureCreated(void);
static void kbClose(bool callCallback);

// --- Profile editor modal ---
static lv_obj_t* s_profModal = nullptr;
static lv_obj_t* s_profSl[5] = {nullptr};
static void profEnsureCreated(void);
static void profOnSlider(lv_event_t* e);
static void profOnClose(lv_event_t* e);

void lvglUiInit(void) {
  lvglScreensInit();
  lvglScreensSetPage(LVGL_NAV_HOME);
  toastEnsureCreated();  // ensure overlays exist
  kbEnsureCreated();
  profEnsureCreated();
}

void lvglUiSetTelemetry(const TelemetryPacket* tm, bool valid) {
  s_hasTelemetry = valid && tm;
  bool eveNow = false;
  if (!s_hasTelemetry) {
    s_data.linkOk = false;
    s_data.batteryV = 0.0f;
    s_data.currentA = 0.0f;
    s_data.mode = 0;
    s_data.eveUartOk = false;
    s_eveUartPrev = false;
    return;
  }
  s_data.batteryV = tm->batteryVoltage;
  s_data.currentA = tm->currentDraw;
  s_data.linkOk = true;
  s_data.mode = tm->autonomyEnabled ? 1 : 0;
  eveNow = (tm->rsv0 & WALLE_TELEM_RSV0_EVE_UART) != 0;
  s_data.eveUartOk = eveNow;

  if (eveNow && !s_eveUartPrev) {
    lvglUiShowToast("EVE companion online", 1600);
    lvglScreensSetPage(LVGL_NAV_EVE);
  } else if (!eveNow && s_eveUartPrev) {
    lvglUiShowToast("EVE link lost", 1400);
  }
  s_eveUartPrev = eveNow;
}

void lvglUiSetJoystick(const JoystickState* js) {
  if (!js) return;
  s_data.joystick = *js;
}

void lvglUiSetDriveState(const DriveState* ds) {
  if (!ds) return;
  s_data.drive = *ds;
}

void lvglUiTick(uint32_t nowMs) {
  lvglScreensTick(&s_data);
  toastUpdate(nowMs);
}

// --- ADDED: SD ready bridge ---

void lvglUiSetSdReady(bool ready) {
  s_sdReady = ready;
}

bool lvglUiIsSdReady(void) {
  return s_sdReady;
}

// --- ADDED: Toast implementation ---

static void toastEnsureCreated(void) {
  if (s_toast) return;
  lv_obj_t* scr = lv_screen_active();
  s_toast = lv_obj_create(scr);
  lv_obj_remove_style_all(s_toast);
  lv_obj_set_size(s_toast, 200, 26);
  lv_obj_set_align(s_toast, LV_ALIGN_BOTTOM_MID);
  lv_obj_set_style_radius(s_toast, 10, 0);
  lv_obj_set_style_bg_color(s_toast, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_toast, LV_OPA_70, 0);
  lv_obj_set_style_border_width(s_toast, 0, 0);
  lv_obj_set_style_pad_all(s_toast, 4, 0);
  lv_obj_add_flag(s_toast, LV_OBJ_FLAG_HIDDEN);

  s_toastLabel = lv_label_create(s_toast);
  lv_label_set_text(s_toastLabel, "");
  lv_obj_set_style_text_color(s_toastLabel, lv_color_white(), 0);
  lv_label_set_long_mode(s_toastLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_width(s_toastLabel, lv_pct(100));

  s_toastVisible = false;
  s_toastUntilMs = 0;
}

void lvglUiShowToast(const char* msg, uint16_t durationMs) {
  if (!msg || !*msg) return;
  toastEnsureCreated();
  if (durationMs < 600) durationMs = 600;
  lv_label_set_text(s_toastLabel, msg);
  lv_obj_clear_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_opa(s_toast, LV_OPA_COVER, 0);
  s_toastVisible = true;
  s_toastUntilMs = millis() + durationMs;
}

static void toastUpdate(uint32_t nowMs) {
  if (!s_toast || !s_toastVisible) return;
  int32_t remain = (int32_t)(s_toastUntilMs - nowMs);
  if (remain <= 0) {
    lv_obj_add_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
    s_toastVisible = false;
    return;
  }
  if (remain < 280) {
    uint8_t opa = (uint8_t)((remain * 255) / 280);
    lv_obj_set_style_opa(s_toast, opa, 0);
  }
}

// --- ADDED: Keyboard implementation ---

static void kbOnEventOk(lv_event_t* e) {
  LV_UNUSED(e);
  kbClose(true);
}

static void kbOnEventCancel(lv_event_t* e) {
  LV_UNUSED(e);
  kbClose(false);
}

static void kbEnsureCreated(void) {
  if (s_kbModal) return;
  lv_obj_t* scr = lv_screen_active();

  s_kbModal = lv_obj_create(scr);
  lv_obj_remove_style_all(s_kbModal);
  lv_obj_set_size(s_kbModal, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(s_kbModal, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_kbModal, LV_OPA_60, 0);
  lv_obj_add_flag(s_kbModal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_flex_flow(s_kbModal, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(s_kbModal, 4, 0);
  lv_obj_set_style_pad_row(s_kbModal, 4, 0);

  lv_obj_t* panel = lv_obj_create(s_kbModal);
  lv_obj_set_size(panel, lv_pct(100), 60);
  lv_obj_set_style_radius(panel, 8, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x111111), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(panel, 4, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(panel, 2, 0);

  s_kbTextarea = lv_textarea_create(panel);
  lv_obj_set_size(s_kbTextarea, lv_pct(100), 26);
  lv_textarea_set_max_length(s_kbTextarea, 48);
  lv_textarea_set_one_line(s_kbTextarea, true);
  lv_obj_set_style_text_color(s_kbTextarea, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_text_color(s_kbTextarea, lv_color_hex(0xBEBEBE), LV_PART_TEXTAREA_PLACEHOLDER);
  lv_obj_set_style_bg_color(s_kbTextarea, LVGL_WALLE_PANEL_2, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(s_kbTextarea, LV_OPA_COVER, LV_PART_MAIN);

  lv_obj_t* row = lv_obj_create(panel);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, lv_pct(100), 24);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row, 6, 0);

  lv_obj_t* btnOk = lv_button_create(row);
  lv_obj_set_size(btnOk, lv_pct(50), 22);
  lv_obj_set_style_bg_color(btnOk, LVGL_WALLE_PANEL_2, 0);
  lv_obj_set_style_bg_opa(btnOk, LV_OPA_COVER, 0);
  lv_obj_t* lblOk = lv_label_create(btnOk);
  lv_label_set_text(lblOk, "OK");
  lv_obj_set_style_text_color(lblOk, lv_color_white(), 0);
  lv_obj_center(lblOk);
  lv_obj_add_event_cb(btnOk, kbOnEventOk, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* btnCancel = lv_button_create(row);
  lv_obj_set_size(btnCancel, lv_pct(50), 22);
  lv_obj_set_style_bg_color(btnCancel, LVGL_WALLE_PANEL_2, 0);
  lv_obj_set_style_bg_opa(btnCancel, LV_OPA_COVER, 0);
  lv_obj_t* lblCancel = lv_label_create(btnCancel);
  lv_label_set_text(lblCancel, "Cancel");
  lv_obj_set_style_text_color(lblCancel, lv_color_white(), 0);
  lv_obj_center(lblCancel);
  lv_obj_add_event_cb(btnCancel, kbOnEventCancel, LV_EVENT_CLICKED, nullptr);

  s_keyboard = lv_keyboard_create(s_kbModal);
  lv_obj_set_size(s_keyboard, lv_pct(100), 140);
  lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(s_keyboard, s_kbTextarea);
  lv_obj_set_style_text_color(s_keyboard, lv_color_white(), LV_PART_ITEMS);
  lv_obj_set_style_text_color(s_keyboard, lv_color_white(), LV_PART_MAIN);
}

static void kbClose(bool callCallback) {
  if (!s_kbModal) return;
  lv_obj_add_flag(s_kbModal, LV_OBJ_FLAG_HIDDEN);
  if (callCallback && s_kbCallback) {
    const char* txt = lv_textarea_get_text(s_kbTextarea);
    s_kbCallback(txt ? txt : "", s_kbUserData);
  }
  s_kbCallback = nullptr;
  s_kbUserData = nullptr;
}

static void profOnSlider(lv_event_t* e) {
  lv_obj_t* s = (lv_obj_t*)lv_event_get_target(e);
  uintptr_t id = (uintptr_t)lv_event_get_user_data(e);
  int v = (int)lv_slider_get_value(s);
  Profile* p = profileGet();
  if (!p) return;
  switch (id) {
    case 0:
      p->joystickDeadzone = constrain(v / 100.0f, 0.0f, 0.5f);
      setJoystickDeadzone(p->joystickDeadzone);
      break;
    case 1:
      p->joystickExpo = constrain(v / 100.0f, 0.0f, 1.0f);
      setJoystickExpo(p->joystickExpo);
      break;
    case 2:
      p->joystickMaxSpeed = constrain(v / 100.0f, 0.0f, 1.0f);
      setJoystickMaxOutput(p->joystickMaxSpeed);
      break;
    case 3: {
      float hs = constrain(0.5f + (v / 100.0f) * 1.5f, 0.5f, 2.0f);
      p->headSensitivity = hs;
      motionSetHeadSensitivity(p->headSensitivity);
      break;
    }
    case 4:
      p->servoSpeedLimit = constrain(v / 100.0f, 0.0f, 1.0f);
      motionSetServoSpeedLimit(p->servoSpeedLimit);
      break;
    default:
      break;
  }
}

static void profOnClose(lv_event_t* e) {
  LV_UNUSED(e);
  if (s_profModal) lv_obj_add_flag(s_profModal, LV_OBJ_FLAG_HIDDEN);
}

static void profEnsureCreated(void) {
  if (s_profModal) return;
  lv_obj_t* scr = lv_screen_active();
  s_profModal = lv_obj_create(scr);
  lv_obj_remove_style_all(s_profModal);
  lv_obj_set_size(s_profModal, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(s_profModal, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(s_profModal, LV_OPA_50, 0);
  lv_obj_add_flag(s_profModal, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* panel = lv_obj_create(s_profModal);
  lv_obj_set_width(panel, 280);
  lv_obj_set_height(panel, LV_SIZE_CONTENT);
  lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel, LVGL_WALLE_PANEL_2, 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel, LVGL_WALLE_BORDER, 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 10, 0);
  lv_obj_set_style_pad_all(panel, 8, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(panel, 4, 0);

  lv_obj_t* title = lv_label_create(panel);
  lv_label_set_text(title, "Profile tuning");
  lv_obj_set_style_text_color(title, LVGL_WALLE_TEXT, 0);

  static const char* names[5] = {"Deadzone", "Expo", "Drive max", "Head sens", "Servo limit"};
  for (int i = 0; i < 5; i++) {
    lv_obj_t* row = lv_obj_create(panel);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 32);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 6, 0);
    lv_obj_t* lab = lv_label_create(row);
    lv_label_set_text(lab, names[i]);
    lv_obj_set_style_text_color(lab, lv_color_white(), 0);
    lv_obj_set_width(lab, 86);
    s_profSl[i] = lv_slider_create(row);
    lv_obj_set_flex_grow(s_profSl[i], 1);
    lv_obj_set_height(s_profSl[i], 12);
    lv_slider_set_range(s_profSl[i], 0, 100);
    lv_obj_set_style_bg_color(s_profSl[i], LVGL_WALLE_PANEL_2, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_profSl[i], LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_profSl[i], LVGL_WALLE_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_profSl[i], LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_profSl[i], LVGL_WALLE_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(s_profSl[i], profOnSlider, LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)i);
  }

  lv_obj_t* close = lv_button_create(panel);
  lv_obj_set_width(close, lv_pct(100));
  lv_obj_set_height(close, 28);
  lv_obj_set_style_bg_color(close, LVGL_WALLE_ACCENT, 0);
  lv_obj_t* cl = lv_label_create(close);
  lv_label_set_text(cl, "Close");
  lv_obj_set_style_text_color(cl, lv_color_black(), 0);
  lv_obj_center(cl);
  lv_obj_add_event_cb(close, profOnClose, LV_EVENT_CLICKED, nullptr);
}

void lvglUiProfileEditorOpen(void) {
  profEnsureCreated();
  Profile* p = profileGet();
  if (!p || !s_profModal) return;
  lv_slider_set_value(s_profSl[0], (int)(p->joystickDeadzone * 100.0f + 0.5f), LV_ANIM_OFF);
  lv_slider_set_value(s_profSl[1], (int)(p->joystickExpo * 100.0f + 0.5f), LV_ANIM_OFF);
  lv_slider_set_value(s_profSl[2], (int)(p->joystickMaxSpeed * 100.0f + 0.5f), LV_ANIM_OFF);
  {
    int hs = (int)(((p->headSensitivity - 0.5f) / 1.5f * 100.0f) + 0.5f);
    lv_slider_set_value(s_profSl[3], constrain(hs, 0, 100), LV_ANIM_OFF);
  }
  lv_slider_set_value(s_profSl[4], (int)(p->servoSpeedLimit * 100.0f + 0.5f), LV_ANIM_OFF);
  lv_obj_clear_flag(s_profModal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_profModal);
}

void lvglUiActionPlayAudioTrack(uint8_t track) {
  if (track == 0) return;
  espnowBroadcastAudioPlayTrack(track);
  playUISound(SOUND_CLICK);
}

void lvglUiShowKeyboard(const char* initialText,
                        LvglKeyboardCallback cb,
                        void* userData) {
  kbEnsureCreated();
  s_kbCallback = cb;
  s_kbUserData = userData;
  lv_obj_clear_flag(s_kbModal, LV_OBJ_FLAG_HIDDEN);
  lv_textarea_set_text(s_kbTextarea, initialText ? initialText : "");
  lv_keyboard_set_textarea(s_keyboard, s_kbTextarea);
}

void lvglUiActionSetUiDrive(int8_t left, int8_t right) {
  lvglInputSetUiDrive(left, right);
}

void lvglUiActionStopAll(void) {
  lvglInputSetUiDrive(0, 0);
  motionEmergencyStop();
  packetSetPendingAction(ACTION_STOP_ALL);
  playUISound(SOUND_ESTOP);
}

void lvglUiActionTriggerAnimation(uint8_t animId) {
  motionTriggerAnimation(animId);
  playUISound(SOUND_CLICK);
}

void lvglUiActionSetHeadTiltPct(int16_t pct) {
  float v = ((float)pct / 50.0f) - 1.0f;
  motionSetHeadTiltVelocity(v);
}

void lvglUiActionSetHeadPanPct(int16_t pct) {
  float v = ((float)pct / 50.0f) - 1.0f;
  motionSetHeadPanVelocity(v);
}

void lvglUiActionSetEyebrowPct(int16_t pct) {
  uint8_t deg = (uint8_t)constrain(map(pct, 0, 100, 40, 140), 0, 180);
  motionSetServoDirect(SERVO_EYEBROW_LEFT, deg);
  motionSetServoDirect(SERVO_EYEBROW_RIGHT, (uint8_t)(180 - deg));
}

void lvglUiActionSetServoSpeedPct(int16_t pct) {
  Profile* p = profileGet();
  if (!p) return;
  float target = constrain((float)pct / 100.0f, 0.0f, 1.0f);
  profileAdjustServoSpeed(target - p->servoSpeedLimit);
}

void lvglUiActionSavePosition(void) {
  uint8_t pos[SERVO_COUNT];
  motionGetServoTargets(pos);
  profileSaveNeutralPositions(pos);
  playUISound(SOUND_CONFIRM);
}

void lvglUiActionLoadPosition(void) {
  profileLoadNeutralPositions();
  playUISound(SOUND_CLICK);
}

void lvglUiActionSetVolumePct(int16_t pct) {
  // Keep existing non-blocking audio path; map slider to ui feedback only.
  if (pct < 30) playUISound(SOUND_CLICK);
  else if (pct < 70) playUISound(SOUND_CONFIRM);
  else playUISound(SOUND_MODE_CHANGE);
}

void lvglUiActionDockGo(void) {
  packetSetPendingAction(ACTION_DOCK_GO);
  playUISound(SOUND_CONFIRM);
  lvglUiShowToast("WALL-E docking requested", 1200);
}

void lvglUiActionDockCancel(void) {
  packetSetPendingAction(ACTION_DOCK_CANCEL);
  playUISound(SOUND_CLICK);
  lvglUiShowToast("Docking cancelled", 1000);
}

void lvglUiActionProfileSet(uint8_t id) {
  profileSet(id);
  playUISound(SOUND_MODE_CHANGE);
}

void lvglUiActionBrightnessSet(uint8_t level255) {
  analogWrite(21, level255);
}

void lvglUiActionToggleJoystick(bool enabled) {
  lvglInputSetJoystickEnabled(enabled);
}

void lvglUiActionSdRefresh(void) {
  if (!s_sdReady) {
    lvglUiShowToast("SD not ready", 900);
    return;
  }
  sdBrowserRefresh();
  playUISound(SOUND_CLICK);
  lvglUiShowToast("SD refreshed", 900);
}

void lvglUiActionSdUp(void) {
  if (!s_sdReady) {
    lvglUiShowToast("SD not ready", 900);
    return;
  }
  sdBrowserGoUp();
  playUISound(SOUND_CLICK);
  lvglUiShowToast("Up", 700);
}

void lvglUiActionEveSendServo(int16_t headPanDeg, int16_t rightArmDeg) {
  uint8_t h = (uint8_t)constrain((int)headPanDeg, 45, 135);
  uint8_t a = (uint8_t)constrain((int)rightArmDeg, 0, 180);
  packetSetEveServo(h, a);
  playUISound(SOUND_CLICK);
}

void lvglUiActionSdOpenSelected(uint16_t idx) {
  if (!s_sdReady) {
    lvglUiShowToast("SD not ready", 900);
    return;
  }
  sdBrowserSetSelected((int16_t)idx);
  const SdDirEntry* e = sdBrowserGetEntry(idx);
  if (!e) {
    lvglUiShowToast("Entry missing", 900);
    return;
  }
  if (e->isDir) {
    if (sdBrowserEnterSelected()) {
      lvglUiShowToast("Folder opened", 900);
    } else {
      lvglUiShowToast("Open failed", 1200);
    }
    playUISound(SOUND_CLICK);
    return;
  }
  // For now: just acknowledge file selection (keyboard kept optional while stabilising).
  playUISound(SOUND_CLICK);
  lvglUiShowToast("File selected", 900);
}
