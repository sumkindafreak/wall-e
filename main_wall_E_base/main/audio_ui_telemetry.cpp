#include "audio_ui_telemetry.h"
#include "menu_protocol.h"
#include <string.h>

static WalleAudioUiTelemPacket_t s_last;
static uint32_t s_lastMs = 0;
static bool s_have = false;

#ifndef AUDIO_UI_STALE_MS
#define AUDIO_UI_STALE_MS 4000u
#endif

void audioUiTelemOnPacket(const uint8_t* data, int len) {
  if (!data || len < (int)sizeof(WalleAudioUiTelemPacket_t)) return;
  const WalleAudioUiTelemPacket_t* p = (const WalleAudioUiTelemPacket_t*)data;
  if (p->magic != WALLE_AUDIO_UI_MAGIC) return;
  if (p->version != WALLE_MENU_PROTO_VERSION) return;
  memcpy(&s_last, p, sizeof(s_last));
  s_lastMs = millis();
  s_have = true;
}

bool audioUiTelemValid(void) {
  if (!s_have) return false;
  return (millis() - s_lastMs) < AUDIO_UI_STALE_MS;
}

uint32_t audioUiTelemLastMs(void) { return s_lastMs; }

uint8_t audioUiTelemGetBtnMode(void) { return s_have ? s_last.btn_mode : 0; }
uint8_t audioUiTelemGetMenuPage(void) { return s_have ? s_last.menu_page : 0; }
uint8_t audioUiTelemGetComboPct(void) { return s_have ? s_last.combo_hold_pct : 0; }
uint8_t audioUiTelemGetLastEvent(void) { return s_have ? s_last.last_ui_event : 0; }

String audioUiTelemGetJSON(void) {
  String j = "{\"ok\":true,\"valid\":";
  j += audioUiTelemValid() ? "true" : "false";
  j += ",\"seq\":"; j += (unsigned)s_last.seq;
  j += ",\"btn_mode\":"; j += (unsigned)s_last.btn_mode;
  j += ",\"menu_page\":"; j += (unsigned)s_last.menu_page;
  j += ",\"voicebox_mode\":"; j += (unsigned)s_last.voicebox_mode;
  j += ",\"last_ui_event\":"; j += (unsigned)s_last.last_ui_event;
  j += ",\"combo_hold_pct\":"; j += (unsigned)s_last.combo_hold_pct;
  j += ",\"menu_sel_idx\":"; j += (unsigned)s_last.menu_sel_idx;
  j += ",\"pair_request\":"; j += (unsigned)s_last.pair_request;
  j += "}";
  return j;
}
