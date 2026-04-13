#include "shared_voicebox_manager.h"
#include "voicebox_protocol.h"
#include "eve_uart_bridge.h"
#include "audio_espnow.h"
#include "relationship_manager.h"
#include "memory_manager.h"
#include "memory_protocol.h"
#include "unified_autonomy_engine.h"

static walle_voicebox_mode_t s_cmdMode = VOICEBOX_SOLO_WALLE;
static uint32_t s_lastVbxMs = 0;

static void routePairToEve(uint8_t pair) {
  if (unifiedAutonomySafetyActive()) {
    Serial.println(F("[SAFETY] Skip EVE pair cue while safety latched"));
    return;
  }
  if (!eveUartBridgeIsLinkUp()) {
    Serial.println(F("[VOICE] EVE offline — cannot pair cue"));
    return;
  }
  uint8_t tr = 3;
  switch (pair) {
    case WALLE_UI_PAIR_EVE_PLAY_ACK:
      tr = 3;
      break;
    case WALLE_UI_PAIR_EVE_STOP_SETTLE:
      tr = 4;
      break;
    case WALLE_UI_PAIR_EVE_REWIND_CONFUSED:
      tr = 5;
      break;
    case WALLE_UI_PAIR_EVE_RECORD_REACT:
      tr = 6;
      break;
    case WALLE_UI_PAIR_MODE_TRANSITION:
      tr = 2;
      break;
    case WALLE_UI_PAIR_EVE_EXPRESSION:
      tr = 5;
      break;
    default:
      return;
  }
  Serial.printf("[VOICE] pair=%u -> EVE track %u\n", (unsigned)pair, (unsigned)tr);
  if (!eveUartBridgeSendPlaySound(tr)) {
    Serial.println(F("[VOICE] UART send failed — WALL-E audio already played"));
  }
}

void sharedVoiceboxInit(void) {
  s_cmdMode = VOICEBOX_SOLO_WALLE;
  s_lastVbxMs = 0;
  Serial.println(F("[VOICE] shared voicebox manager init"));
}

void sharedVoiceboxOnAudioUi(const WalleAudioUiTelemPacket_t* pkt) {
  if (!pkt || pkt->pair_request == WALLE_UI_PAIR_NONE) return;
  routePairToEve(pkt->pair_request);
}

void sharedVoiceboxTick(uint32_t nowMillis) {
  bool eve = eveUartBridgeIsLinkUp();
  walle_voicebox_mode_t want = eve ? VOICEBOX_SHARED_WALLE_EVE : VOICEBOX_SOLO_WALLE;
  uint8_t bond = relationshipGetBondStrength();

  if (want != s_cmdMode) {
    if (want == VOICEBOX_SOLO_WALLE) {
      Serial.println(F("[VOICE] EVE offline, falling back to SOLO_WALLE"));
      memoryManagerLog(WALLE_MEM_EV_VOICEBOX_MODE, "solo_fallback");
      memoryManagerPersistVoiceboxMode("SOLO_WALLE");
    } else {
      Serial.println(F("[VOICE] Shared voice box active"));
      memoryManagerLog(WALLE_MEM_EV_VOICEBOX_MODE, "shared_active");
      memoryManagerPersistVoiceboxMode("SHARED_WALLE_EVE");
    }
    s_cmdMode = want;
  }

  if (nowMillis - s_lastVbxMs >= 480u) {
    s_lastVbxMs = nowMillis;
    uint8_t m = (uint8_t)s_cmdMode;
    uint8_t ok = (eve && s_cmdMode == VOICEBOX_SHARED_WALLE_EVE) ? 1u : 0u;
    (void)audioEspNowSendVoiceboxCmd(m, ok, bond);
  }
}

const char* sharedVoiceboxModeName(void) {
  return s_cmdMode == VOICEBOX_SHARED_WALLE_EVE ? "SHARED_WALLE_EVE" : "SOLO_WALLE";
}

bool sharedVoiceboxIsShared(void) {
  return s_cmdMode == VOICEBOX_SHARED_WALLE_EVE && eveUartBridgeIsLinkUp();
}
