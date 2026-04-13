#include "button_menu_manager.h"
#include "pins.h"
#include "config.h"
#include "audio_player.h"
#include "menu_state_machine.h"
#include "dfplayer_character_controls.h"

static const int kBtnPins[4] = {PIN_BTN_1, PIN_BTN_2, PIN_BTN_3, PIN_BTN_4};

static bool s_menuMode = false;
static bool s_comboWasHeld = false;
static uint32_t s_comboStartMs = 0;
static uint32_t s_lastComboBeepMs = 0;
static uint8_t s_comboPct = 0;
static bool s_needComboRelease = false;

static bool s_stable[4];
static uint32_t s_lastChangeMs[4];

static walle_ui_event_t s_uiEvent = WALLE_UI_EVT_NONE;

static bool rawDown(unsigned i) {
  if (i >= 4) return false;
  return digitalRead(kBtnPins[i]) == LOW;
}

static void latchEvent(walle_ui_event_t e) { s_uiEvent = e; }

void buttonMenuInit(void) {
  for (unsigned i = 0; i < 4; i++) {
    pinMode(kBtnPins[i], INPUT_PULLUP);
    s_stable[i] = false;
    s_lastChangeMs[i] = 0;
  }
  s_menuMode = false;
  s_comboWasHeld = false;
  s_comboStartMs = 0;
  s_lastComboBeepMs = 0;
  s_comboPct = 0;
  s_needComboRelease = false;
  s_uiEvent = WALLE_UI_EVT_NONE;
  menuStateInit();
  Serial.println(F("[BTN] Character buttons init (pull-up, active LOW)"));
}

bool buttonMenuIsMenuMode(void) { return s_menuMode; }
uint8_t buttonMenuGetComboHoldPct(void) { return s_comboPct; }
walle_ui_event_t buttonMenuGetLastUiEvent(void) { return s_uiEvent; }
void buttonMenuClearUiEvent(void) { s_uiEvent = WALLE_UI_EVT_NONE; }

static void updateDebounce(unsigned long now) {
  for (unsigned i = 0; i < 4; i++) {
    bool raw = rawDown(i);
    if (raw != s_stable[i]) {
      if (s_lastChangeMs[i] == 0) s_lastChangeMs[i] = (uint32_t)now;
      if ((uint32_t)now - s_lastChangeMs[i] >= BTN_DEBOUNCE_MS) {
        s_stable[i] = raw;
        s_lastChangeMs[i] = 0;
      }
    } else {
      s_lastChangeMs[i] = 0;
    }
  }
}

static bool comboPhysicalHeld(void) {
  return rawDown(0) && rawDown(1); /* BTN1+BTN2 pins */
}

static void toggleMenu(unsigned long now) {
  s_menuMode = !s_menuMode;
  s_comboPct = 0;
  s_needComboRelease = true;
  if (s_menuMode) {
    menuStateInit();
    Serial.println(F("[MENU] Entered (6s combo)"));
    latchEvent(WALLE_UI_EVT_MENU_ENTER);
    audioPlayTrack(TRACK_MENU_ENTER_OK, PRIO_MENU);
  } else {
    Serial.println(F("[MENU] Exit combo hold complete"));
    latchEvent(WALLE_UI_EVT_MENU_EXIT);
    audioPlayTrack(TRACK_MENU_EXIT_OK, PRIO_MENU);
  }
  (void)now;
}

void buttonMenuTick(unsigned long now) {
  updateDebounce(now);
  menuStateTick(now);

  bool ch = comboPhysicalHeld();
  if (s_needComboRelease) {
    if (!ch) s_needComboRelease = false;
    s_comboPct = 0;
    s_comboWasHeld = false;
    return;
  }

  if (ch) {
    if (!s_comboWasHeld) {
      s_comboWasHeld = true;
      s_comboStartMs = (uint32_t)now;
      s_lastComboBeepMs = (uint32_t)now;
    }
    uint32_t held = (uint32_t)now - s_comboStartMs;
    s_comboPct = (uint8_t)((held >= MENU_COMBO_HOLD_MS) ? 100u : (held * 100u / MENU_COMBO_HOLD_MS));
    if ((uint32_t)now - s_lastComboBeepMs >= MENU_COMBO_BEEP_MS) {
      s_lastComboBeepMs = (uint32_t)now;
      audioPlayTrack(TRACK_MENU_TICK, PRIO_LOWEST);
    }
    if (held >= MENU_COMBO_HOLD_MS) {
      toggleMenu(now);
      s_comboWasHeld = false;
      s_comboStartMs = (uint32_t)now;
    }
    return;
  }

  s_comboWasHeld = false;
  s_comboPct = 0;
  s_lastComboBeepMs = (uint32_t)now;

  /* Normal or menu: edges on debounced stable states */
  static bool s_prev[4] = {false, false, false, false};
  for (unsigned i = 0; i < 4; i++) {
    if (s_stable[i] && !s_prev[i]) {
      /* press */
      if (s_menuMode) {
        menuStateOnButton((uint8_t)(i + 1u), now);
        latchEvent(WALLE_UI_EVT_MENU_NAV);
        menuStateResetTimeout(now);
      } else {
        switch (i) {
          case 0:
            latchEvent(WALLE_UI_EVT_BTN_PLAY);
            charControlsPlayButton();
            break;
          case 1:
            latchEvent(WALLE_UI_EVT_BTN_STOP);
            charControlsStopButton();
            break;
          case 2:
            latchEvent(WALLE_UI_EVT_BTN_REWIND);
            charControlsRewindButton();
            break;
          case 3:
            latchEvent(WALLE_UI_EVT_BTN_RECORD);
            charControlsRecordButton();
            break;
          default:
            break;
        }
      }
    }
    s_prev[i] = s_stable[i];
  }
}
