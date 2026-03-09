// ============================================================
//  WALL-E Dock Controller — ESP-NOW commands to dock
// ============================================================

#include "dock_controller.h"
#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>
#include <cstring>

#define DOCK_CMD_MAGIC  0x434D444B
#define DOCK_CMD_FORCE_OFF  1
#define DOCK_CMD_RESET      2
#define DOCK_CMD_WIFI_CONFIG 3
#define DOCK_CMD_TIME 4

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t dock_id;
  uint8_t  cmd;
  uint8_t  pad[3];
} DockCommandPacket_t;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t dock_id;
  uint8_t  cmd;
  uint8_t  pad[3];
  char     ssid[33];
  char     pass[65];
} DockWifiConfigPacket_t;

static uint8_t s_broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static bool s_peer_added = false;

static bool ensureBroadcastPeer(void) {
  if (s_peer_added) return true;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, s_broadcast_mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_AP;
  esp_err_t r = esp_now_add_peer(&peer);
  if (r == ESP_OK || r == ESP_ERR_ESPNOW_EXIST) {
    s_peer_added = true;
    return true;
  }
  return false;
}

static bool sendCmd(uint32_t dock_id, uint8_t cmd) {
  if (!ensureBroadcastPeer()) return false;
  DockCommandPacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = cmd;
  esp_err_t r = esp_now_send(s_broadcast_mac, (uint8_t*)&pkt, sizeof(pkt));
  return (r == ESP_OK);
}

bool dockControllerSendWifiConfig(const char* ssid, const char* pass, uint32_t dock_id) {
  if (!ssid || strlen(ssid) == 0 || strlen(ssid) > 32) return false;
  if (!ensureBroadcastPeer()) return false;
  DockWifiConfigPacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = DOCK_CMD_WIFI_CONFIG;
  strncpy(pkt.ssid, ssid, 32);
  pkt.ssid[32] = '\0';
  if (pass) strncpy(pkt.pass, pass, 64);
  pkt.pass[64] = '\0';
  esp_err_t r = esp_now_send(s_broadcast_mac, (uint8_t*)&pkt, sizeof(pkt));
  return (r == ESP_OK);
}

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t dock_id;
  uint8_t  cmd;
  uint8_t  pad[3];
  uint32_t unix_time;
} DockTimePacket_t;

bool dockControllerSendTime(uint32_t unix_time, uint32_t dock_id) {
  if (!ensureBroadcastPeer()) return false;
  DockTimePacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = DOCK_CMD_TIME;
  pkt.unix_time = unix_time;
  esp_err_t r = esp_now_send(s_broadcast_mac, (uint8_t*)&pkt, sizeof(pkt));
  return (r == ESP_OK);
}

void dockControllerInit(void) {
  /* Peer added on first send */
}

bool dockControllerSendForceOff(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_FORCE_OFF);
}

bool dockControllerSendReset(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_RESET);
}
