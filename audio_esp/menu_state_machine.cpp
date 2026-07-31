#include "menu_state_machine.h"
#include "audio_player.h"
#include "voicebox_router.h"
#include "config.h"
#include <Arduino.h>

static walle_menu_page_t s_page = WALLE_MENU_PAGE_STATUS;
static uint8_t s_sel = 0;
static uint32_t s_lastActMs = 0;
static uint32_t s_rebootArmMs = 0;

static const char* pageName(walle_menu_page_t p) {
  switch (p) {
    case WALLE_MENU_PAGE_STATUS: return "STATUS";
    case WALLE_MENU_PAGE_AUDIO_TEST: return "AUDIO TEST";
    case WALLE_MENU_PAGE_EXPRESSIONS: return "EXPRESSIONS";
    case WALLE_MENU_PAGE_DOCK_STATUS: return "DOCK STATUS";
    case WALLE_MENU_PAGE_EVE_LINK: return "EVE LINK";
    case WALLE_MENU_PAGE_VOICE_BOX: return "VOICE BOX";
    case WALLE_MENU_PAGE_MEMORY_LOG: return "MEMORY LOG";
    case WALLE_MENU_PAGE_SYSTEM_INFO: return "SYSTEM INFO";
    case WALLE_MENU_PAGE_SAFE_REBOOT: return "SAFE REBOOT";
    default: return "?";
  }
}

void menuStateInit(void) {
  s_page = WALLE_MENU_PAGE_STATUS;
  s_sel = 0;
  s_lastActMs = millis();
  s_rebootArmMs = 0;
}

void menuStateResetTimeout(unsigned long now) { s_lastActMs = (uint32_t)now; }

void menuStateTick(unsigned long now) {
  if ((uint32_t)now - s_lastActMs > MENU_PAGE_TIMEOUT_MS) {
    if (s_page != WALLE_MENU_PAGE_STATUS) {
      s_page = WALLE_MENU_PAGE_STATUS;
      s_sel = 0;
      Serial.println(F("[MENU] timeout -> STATUS"));
    }
  }
}

walle_menu_page_t menuStateGetPage(void) { return s_page; }
uint8_t menuStateGetSel(void) { return s_sel; }

static void beepNav(void) { audioPlayTrack(TRACK_MENU_TICK, PRIO_MENU); }

static void runAudioTestSelect(void) {
  /* 0 = WALL-E track, 1 = EVE cue via pair req, 2 = pair sequence */
  if (s_sel == 0) {
    Serial.println(F("[MENU] Audio test: WALL-E only"));
    audioPlayTrack(TRACK_CHAR_PLAY, PRIO_MENU);
  } else if (s_sel == 1) {
    Serial.println(F("[MENU] Audio test: EVE path (via base)"));
    voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_PLAY_ACK);
  } else {
    Serial.println(F("[MENU] Audio test: shared pair"));
    audioPlayTrack(TRACK_CHAR_PLAY, PRIO_MENU);
    voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_PLAY_ACK);
  }
}

static void runVoiceBoxPageSelect(void) {
  if (s_sel == 0) {
    Serial.print(F("[MENU] Voice box mode: "));
    Serial.println(voiceboxRouterGetMode() == VOICEBOX_SHARED_WALLE_EVE ? F("SHARED") : F("SOLO_WALLE"));
  } else if (s_sel == 1) {
    audioPlayTrack(TRACK_CHAR_PLAY, PRIO_MENU);
    voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_PLAY_ACK);
  } else {
    audioPlayTrack(TRACK_MENU_ENTER_OK, PRIO_MENU);
  }
}

void menuStateOnButton(uint8_t menuBtn, unsigned long now) {
  s_lastActMs = (uint32_t)now;

  if (menuBtn == 1) { /* up / prev */
    if (s_page == WALLE_MENU_PAGE_AUDIO_TEST || s_page == WALLE_MENU_PAGE_VOICE_BOX) {
      if (s_sel > 0) s_sel--;
    } else if (s_page > WALLE_MENU_PAGE_STATUS) {
      s_page = (walle_menu_page_t)((int)s_page - 1);
      s_sel = 0;
    }
    Serial.print(F("[MENU] "));
    Serial.println(pageName(s_page));
    beepNav();
    return;
  }
  if (menuBtn == 2) { /* down / next */
    if (s_page == WALLE_MENU_PAGE_AUDIO_TEST || s_page == WALLE_MENU_PAGE_VOICE_BOX) {
      if (s_sel < 2) s_sel++;
    } else if (s_page < (WALLE_MENU_PAGE_COUNT - 1)) {
      s_page = (walle_menu_page_t)((int)s_page + 1);
      s_sel = 0;
    }
    Serial.print(F("[MENU] "));
    Serial.println(pageName(s_page));
    beepNav();
    return;
  }
  if (menuBtn == 4) { /* back */
    s_page = WALLE_MENU_PAGE_STATUS;
    s_sel = 0;
    Serial.println(F("[MENU] Back -> STATUS"));
    beepNav();
    return;
  }
  /* select */
  if (menuBtn != 3) return;

  Serial.print(F("[MENU] Entered "));
  Serial.println(pageName(s_page));

  switch (s_page) {
    case WALLE_MENU_PAGE_STATUS:
      Serial.print(F("[MENU] STATUS heap="));
      Serial.println(ESP.getFreeHeap());
      break;
    case WALLE_MENU_PAGE_AUDIO_TEST:
      runAudioTestSelect();
      break;
    case WALLE_MENU_PAGE_EXPRESSIONS:
      voiceboxRouterSetPairRequest(WALLE_UI_PAIR_EVE_EXPRESSION);
      audioPlayTrack(TRACK_CURIOUS, PRIO_MENU);
      break;
    case WALLE_MENU_PAGE_DOCK_STATUS:
      audioPlayTrack(TRACK_DOCK_GUIDE, PRIO_MENU);
      break;
    case WALLE_MENU_PAGE_EVE_LINK:
      Serial.println(F("[MENU] EVE link: see base telemetry"));
      audioPlayTrack(TRACK_ACK, PRIO_MENU);
      break;
    case WALLE_MENU_PAGE_VOICE_BOX:
      runVoiceBoxPageSelect();
      break;
    case WALLE_MENU_PAGE_MEMORY_LOG:
      Serial.println(F("[MENU] MEMORY LOG: use Web UI / LittleFS on base"));
      audioPlayTrack(TRACK_ACK, PRIO_MENU);
      break;
    case WALLE_MENU_PAGE_SYSTEM_INFO:
      Serial.printf("[MENU] SYSTEM uptime_ms=%lu dfplayer_ok=%d\n", (unsigned long)now, audioIsReady() ? 1 : 0);
      break;
    case WALLE_MENU_PAGE_SAFE_REBOOT:
      if (s_rebootArmMs == 0) {
        s_rebootArmMs = (uint32_t)now;
        Serial.println(F("[MENU] SAFE REBOOT armed — SELECT again within 8s"));
        audioPlayTrack(TRACK_ERROR, PRIO_MENU);
      } else if ((uint32_t)now - s_rebootArmMs < 8000u) {
        Serial.println(F("[BOOT] SAFE REBOOT confirmed"));
        audioPlayTrack(TRACK_MENU_EXIT_OK, PRIO_MENU);
        delay(300); /* allow DFPlayer to start */
        ESP.restart();
      } else {
        s_rebootArmMs = 0;
        Serial.println(F("[MENU] SAFE REBOOT arm expired"));
      }
      break;
    default:
      break;
  }
}
