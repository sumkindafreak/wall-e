/**
 * audio_esp_status.cpp — Store and expose Audio ESP status
 */
#include "audio_esp_status.h"
#include "audio_protocol.h"
#include <Arduino.h>
#include <cstring>

#define STATUS_TIMEOUT_MS 3000

static uint8_t s_mic_dir = 0;
static uint8_t s_dock_ir = 0;
static uint8_t s_voice_cmd = 0;
static uint8_t s_mode = 0;
static uint8_t s_fault = 0;
static uint8_t s_audio_busy = 0;
static uint32_t s_last_ms = 0;

void audioEspStatusOnPacket(const void* data, int len) {
  if (!data || len < (int)sizeof(WalleAudioStatusPacket_t)) return;
  const WalleAudioStatusPacket_t* p = (const WalleAudioStatusPacket_t*)data;
  if (p->magic != WALLE_AUDIO_STATUS_MAGIC) return;

  s_mic_dir = p->mic_dir;
  s_dock_ir = p->dock_ir;
  s_voice_cmd = p->voice_cmd;
  s_mode = p->mode;
  s_fault = p->fault;
  s_audio_busy = p->audio_busy;
  s_last_ms = millis();
}

bool audioEspStatusValid(void) {
  return s_last_ms != 0 && (millis() - s_last_ms) < STATUS_TIMEOUT_MS;
}

uint8_t audioEspStatusGetMicDir(void)   { return s_mic_dir; }
uint8_t audioEspStatusGetDockIr(void)   { return s_dock_ir; }
uint8_t audioEspStatusGetVoiceCmd(void) { return s_voice_cmd; }
uint8_t audioEspStatusGetMode(void)     { return s_mode; }
uint8_t audioEspStatusGetFault(void)    { return s_fault; }
uint8_t audioEspStatusGetAudioBusy(void) { return s_audio_busy; }

int8_t audioEspStatusGetDockBalance(void) {
  switch (s_dock_ir) {
    case WALLE_AU_DOCK_IR_LEFT:    return -50;
    case WALLE_AU_DOCK_IR_RIGHT:   return  50;
    case WALLE_AU_DOCK_IR_BOTH:    return 0;
    case WALLE_AU_DOCK_IR_UNSTABLE: return 0;
    default:                       return 0;
  }
}
