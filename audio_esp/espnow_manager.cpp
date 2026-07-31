/**
 * espnow_manager.cpp — ESP-NOW to Base
 */
#include "espnow_manager.h"
#include "audio_player.h"
#include <esp_log.h>
#include "system_state.h"
#include "mic_manager.h"
#include "ir_dock_receivers.h"
#include "voice_commands.h"
#include "config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

#include "audio_protocol.h"
#include "node_health_protocol.h"
#include "voicebox_protocol.h"
#include "menu_protocol.h"
#include "voicebox_router.h"

#define WALLE_ESPNOW_WIFI_CHANNEL 11

static uint8_t s_bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool s_init = false;
static bool s_peer_ok = false;
static unsigned long s_lastMicTelemMs = 0;
static unsigned long s_lastStatusMs = 0;
static unsigned long s_lastHealthMs = 0;
static unsigned long s_lastUiMs = 0;
#define MIC_TELEM_INTERVAL_MS  100
#define STATUS_INTERVAL_MS     200
#define NODE_HEALTH_INTERVAL_MS 900
#define UI_TELEM_INTERVAL_MS   180

static bool ensurePeer(void) {
  if (s_peer_ok) return true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_bcast, 6);
  peer.channel = WALLE_ESPNOW_WIFI_CHANNEL;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  esp_err_t r = esp_now_add_peer(&peer);
  if (r == ESP_OK || r == ESP_ERR_ESPNOW_EXIST) {
    s_peer_ok = true;
    return true;
  }
  return false;
}

/* Map event to track — for WALLE_AU_CMD_PLAY_EVENT */
static uint8_t eventToTrack(uint8_t evt) {
  switch (evt) {
    case WALLE_AUDIO_EVT_CURIOUS: return 5;  /* TRACK_CURIOUS */
    case WALLE_AUDIO_EVT_ESTOP:   return 9;  /* TRACK_STOP */
    case WALLE_AUDIO_EVT_IDLE:    return 2;  /* TRACK_HELLO */
    case WALLE_AUDIO_EVT_BOOT:    return 1;  /* TRACK_STARTUP */
    default:                      return 0;
  }
}

/* Handle incoming WalleAudioCommandPacket_t */
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  if (len >= (int)sizeof(WalleVoiceboxCmdPacket_t)) {
    const WalleVoiceboxCmdPacket_t* vx = (const WalleVoiceboxCmdPacket_t*)data;
    if (vx->magic == WALLE_VOICEBOX_CMD_MAGIC) {
      voiceboxRouterApplyCmd(data, len);
      return;
    }
  }
  if (len < (int)sizeof(WalleAudioCommandPacket_t)) return;
  const WalleAudioCommandPacket_t* p = (const WalleAudioCommandPacket_t*)data;
  if (p->magic != WALLE_AUDIO_MAGIC) return;

  DEBUG_COMMS_LOG("RX cmd=%u param=%u", (unsigned)p->cmd, (unsigned)p->param);

  switch (p->cmd) {
    case WALLE_AU_CMD_PLAY_TRACK:
      if (p->param >= 1 && p->param <= 255)
        audioPlayTrack(p->param, p->priority > 0 ? p->priority : 10);
      break;
    case WALLE_AU_CMD_PLAY_EVENT: {
      uint8_t track = eventToTrack(p->param);
      if (track > 0)
        audioPlayTrack(track, p->priority > 0 ? p->priority : 10);
      break;
    }
    case WALLE_AU_CMD_VOLUME:
      audioSetVolume(p->param <= 30 ? (uint8_t)(p->param * 100 / 30) : p->param);
      break;
    case WALLE_AU_CMD_STOP:
      audioStop();
      break;
    default:
      break;
  }
}

static void onSend(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
  (void)tx_info;
  (void)status;
}

bool espnowManagerInit(void) {
  if (s_init) return true;
  esp_log_level_set("ESPNOW", ESP_LOG_NONE);  /* suppress peer channel errors */
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);
  /* Match Base channel — ignore failure (e.g. no Base nearby) */
  (void)esp_wifi_set_channel(WALLE_ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    s_init = false;
    return false;  /* No ESP-NOW — boot continues, sends will no-op */
  }
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSend);
  s_init = true;
  s_peer_ok = false;
  s_lastMicTelemMs = 0;
  s_lastStatusMs = 0;
  DEBUG_LOG("ESP-NOW ready (channel %d)", WALLE_ESPNOW_WIFI_CHANNEL);
  return true;
}

void espnowManagerTick(void) {
  /* Nothing to poll; recv is callback-driven */
}

bool espnowManagerIsReady(void) { return s_init; }

void espnowManagerSendMicTelem(uint16_t ear_l, uint16_t ear_r, uint8_t voice_active) {
  if (!s_init || !ensurePeer()) return;
  unsigned long now = millis();
  if (now - s_lastMicTelemMs < MIC_TELEM_INTERVAL_MS) return;
  s_lastMicTelemMs = now;

  WalleAudioMicTelemPacket_t pkt = {
    .magic = WALLE_AUDIO_MIC_MAGIC,
    .ear_l = ear_l,
    .ear_r = ear_r,
    .voice_active = voice_active,
    .reserved = 0,
  };
  esp_now_send(s_bcast, (uint8_t*)&pkt, sizeof(pkt));
}

void espnowManagerSendStatus(uint8_t mic_dir, uint8_t dock_ir, uint8_t voice_cmd,
                             uint8_t mode, uint8_t fault, uint8_t audio_busy) {
  if (!s_init || !ensurePeer()) return;
  unsigned long now = millis();
  if (now - s_lastStatusMs < STATUS_INTERVAL_MS) return;
  s_lastStatusMs = now;

  WalleAudioStatusPacket_t pkt = {};
  pkt.magic = WALLE_AUDIO_STATUS_MAGIC;
  pkt.mic_dir = mic_dir;
  pkt.dock_ir = dock_ir;
  pkt.voice_cmd = voice_cmd;
  pkt.mode = mode;
  pkt.fault = fault;
  pkt.audio_busy = audio_busy;
  esp_now_send(s_bcast, (uint8_t*)&pkt, sizeof(pkt));
}

void espnowManagerSendNodeHealth(void) {
  if (!s_init || !ensurePeer()) return;
  unsigned long now = millis();
  if (now - s_lastHealthMs < NODE_HEALTH_INTERVAL_MS) return;
  s_lastHealthMs = now;
  WalleNodeHealthPacket_t h = {};
  h.magic = WALLE_NODE_HEALTH_MAGIC;
  h.version = WALLE_NODE_HEALTH_VERSION;
  h.node_id = WALLE_NODE_AUDIO;
  h.role = WALLE_ROLE_ACTOR;
  h.battery_pct = WALLE_NODE_HEALTH_UNKNOWN_BAT;
  h.temp_c = WALLE_NODE_HEALTH_UNKNOWN_TEMP;
  h.uptime_ms = (uint32_t)now;
  h.last_error = 0;
  h.flags = 0;
  h.free_heap = ESP.getFreeHeap();
  h.loop_p95_ms = 0;
  h.wifi_rssi = (int8_t)WiFi.RSSI();
  h.sd_ok = audioIsReady() ? 1 : 0;
  h.last_peer_rx_age_ms = 0xFFFFFFFFu;
  esp_now_send(s_bcast, (uint8_t*)&h, sizeof(h));
}

bool espnowManagerSendUiTelem(const WalleAudioUiTelemPacket_t* pkt, bool force) {
  if (!s_init || !pkt || !ensurePeer()) return false;
  unsigned long now = millis();
  if (!force && (now - s_lastUiMs < UI_TELEM_INTERVAL_MS)) return false;
  s_lastUiMs = now;
  WalleAudioUiTelemPacket_t copy = *pkt;
  copy.magic = WALLE_AUDIO_UI_MAGIC;
  if (copy.version == 0) copy.version = WALLE_MENU_PROTO_VERSION;
  esp_err_t e = esp_now_send(s_bcast, (uint8_t*)&copy, sizeof(copy));
  return e == ESP_OK;
}
