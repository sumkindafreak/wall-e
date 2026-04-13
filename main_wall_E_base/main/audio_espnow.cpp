#include "audio_espnow.h"
#include "audio_protocol.h"
#include "voicebox_protocol.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

/* Match audio_esp espnow_manager / CYD — Base AP uses same channel for ESP-NOW */
#ifndef WALLE_ESPNOW_WIFI_CHANNEL
#define WALLE_ESPNOW_WIFI_CHANNEL 11
#endif

static uint8_t s_bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_peer_ok = false;

static bool ensureBroadcastPeer(void) {
  if (s_peer_ok) {
    return true;
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_bcast, 6);
  peer.channel = WALLE_ESPNOW_WIFI_CHANNEL;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_AP;
  esp_err_t r = esp_now_add_peer(&peer);
  if (r == ESP_OK || r == ESP_ERR_ESPNOW_EXIST) {
    s_peer_ok = true;
    return true;
  }
  return false;
}

static bool sendPacket(uint8_t cmd, uint8_t param, uint8_t priority) {
  if (!ensureBroadcastPeer()) {
    return false;
  }
  WalleAudioCommandPacket_t pkt = {
      .magic = WALLE_AUDIO_MAGIC,
      .cmd = cmd,
      .param = param,
      .reserved = 0,
      .priority = priority,
  };
  return esp_now_send(s_bcast, (uint8_t *)&pkt, sizeof(pkt)) == ESP_OK;
}

void audioEspNowInit(void) {}

bool audioEspNowPlayTrack(uint8_t track, uint8_t priority) {
  if (track < 1) {
    return false;
  }
  return sendPacket(WALLE_AU_CMD_PLAY_TRACK, track, priority);
}

bool audioEspNowSetVolume(uint8_t vol_dfplayer_0_30) {
  uint8_t v = vol_dfplayer_0_30 > 30 ? 30 : vol_dfplayer_0_30;
  return sendPacket(WALLE_AU_CMD_VOLUME, v, 0);
}

bool audioEspNowSendEvent(uint8_t event_id, uint8_t priority) {
  return sendPacket(WALLE_AU_CMD_PLAY_EVENT, event_id, priority);
}

bool audioEspNowStop(void) {
  return sendPacket(WALLE_AU_CMD_STOP, 0, 0);
}

bool audioEspNowSendVoiceboxCmd(uint8_t mode, uint8_t eve_audio_ok, uint8_t bond_strength) {
  if (!ensureBroadcastPeer()) {
    return false;
  }
  WalleVoiceboxCmdPacket_t pkt = {};
  pkt.magic = WALLE_VOICEBOX_CMD_MAGIC;
  pkt.version = WALLE_VOICEBOX_PROTO_VERSION;
  pkt.mode = mode;
  pkt.eve_audio_ok = eve_audio_ok ? 1u : 0u;
  pkt.bond_strength = bond_strength;
  return esp_now_send(s_bcast, (uint8_t*)&pkt, sizeof(pkt)) == ESP_OK;
}
