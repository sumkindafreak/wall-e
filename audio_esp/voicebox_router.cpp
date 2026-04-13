#include "voicebox_router.h"

static walle_voicebox_mode_t s_mode = VOICEBOX_SOLO_WALLE;
static uint8_t s_eveOk = 0;
static uint8_t s_bond = 0;
static walle_ui_pair_request_t s_pair = WALLE_UI_PAIR_NONE;

void voiceboxRouterInit(void) {
  s_mode = VOICEBOX_SOLO_WALLE;
  s_eveOk = 0;
  s_bond = 0;
  s_pair = WALLE_UI_PAIR_NONE;
  Serial.println(F("[VOICE] VOICEBOX_SOLO_WALLE (boot)"));
}

void voiceboxRouterApplyCmd(const uint8_t* data, int len) {
  if (!data || len < (int)sizeof(WalleVoiceboxCmdPacket_t)) return;
  const WalleVoiceboxCmdPacket_t* p = (const WalleVoiceboxCmdPacket_t*)data;
  if (p->magic != WALLE_VOICEBOX_CMD_MAGIC) return;
  if (p->version != WALLE_VOICEBOX_PROTO_VERSION) {
    Serial.println(F("[VOICE] reject VBXC version"));
    return;
  }
  walle_voicebox_mode_t newMode =
      (p->mode == (uint8_t)VOICEBOX_SHARED_WALLE_EVE) ? VOICEBOX_SHARED_WALLE_EVE : VOICEBOX_SOLO_WALLE;
  uint8_t newEve = p->eve_audio_ok ? 1u : 0u;
  if (newMode != s_mode) {
    s_mode = newMode;
    Serial.print(F("[VOICE] mode -> "));
    Serial.println(s_mode == VOICEBOX_SHARED_WALLE_EVE ? F("SHARED_WALLE_EVE") : F("SOLO_WALLE"));
    if (s_mode == VOICEBOX_SHARED_WALLE_EVE && newEve)
      s_pair = WALLE_UI_PAIR_MODE_TRANSITION;
  }
  s_eveOk = newEve;
  s_bond = p->bond_strength;
  if (s_mode == VOICEBOX_SHARED_WALLE_EVE && !s_eveOk) {
    Serial.println(F("[VOICE] SHARED mode but EVE audio path offline — WALL-E only"));
  }
}

walle_voicebox_mode_t voiceboxRouterGetMode(void) { return s_mode; }
uint8_t voiceboxRouterGetEveAudioOk(void) { return s_eveOk; }
uint8_t voiceboxRouterGetBondStrength(void) { return s_bond; }

void voiceboxRouterSetPairRequest(walle_ui_pair_request_t req) {
  if (req != WALLE_UI_PAIR_NONE) s_pair = req;
}

walle_ui_pair_request_t voiceboxRouterPeekPairRequest(void) { return s_pair; }

void voiceboxRouterClearPairRequest(void) { s_pair = WALLE_UI_PAIR_NONE; }

bool voiceboxRouterIsSharedActive(void) {
  return (s_mode == VOICEBOX_SHARED_WALLE_EVE) && s_eveOk;
}
