#include "audio_telem.h"
#include "audio_protocol.h"
#include <Arduino.h>
#include <cstring>

static uint16_t s_ear_l = 0, s_ear_r = 0;
static uint8_t s_voice = 0;
static uint32_t s_last_ms = 0;

void audioTelemOnPacket(const uint8_t *data, int len) {
  if (!data || len != (int)sizeof(WalleAudioMicTelemPacket_t)) {
    return;
  }
  const WalleAudioMicTelemPacket_t *p = (const WalleAudioMicTelemPacket_t *)data;
  if (p->magic != WALLE_AUDIO_MIC_MAGIC) {
    return;
  }
  s_ear_l = p->ear_l;
  s_ear_r = p->ear_r;
  s_voice = p->voice_active;
  s_last_ms = millis();
}

bool audioTelemGet(uint16_t *ear_l, uint16_t *ear_r, uint8_t *voice_active, uint32_t *age_ms) {
  uint32_t now = millis();
  uint32_t age = (s_last_ms == 0) ? 999999u : (now - s_last_ms);
  if (ear_l) {
    *ear_l = s_ear_l;
  }
  if (ear_r) {
    *ear_r = s_ear_r;
  }
  if (voice_active) {
    *voice_active = s_voice;
  }
  if (age_ms) {
    *age_ms = age;
  }
  return s_last_ms != 0 && age < 3000u;
}
