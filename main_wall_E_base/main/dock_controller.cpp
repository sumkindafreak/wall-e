// ============================================================
//  WALL-E Dock Controller — ESP-NOW commands to dock
// ============================================================

#include "dock_controller.h"
#include "dock_protocol.h"
#include "walle_link_packet.h"
#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h>
#include <cstring>

static_assert(sizeof(DockCommandPacket_t) == 12, "DockCommandPacket_t wire-size drift");
static_assert(sizeof(DockApproachStagePacket_t) == 12, "DockApproachStagePacket_t wire-size drift");
static_assert(sizeof(DockTimePacket_t) == 16, "DockTimePacket_t wire-size drift");
static_assert(sizeof(DockWifiConfigPacket_t) == 110, "DockWifiConfigPacket_t wire-size drift");

static uint8_t s_broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static bool s_peer_added = false;

static uint32_t s_lastApproachSendOkMs = 0;
static uint32_t s_lastChargeSendOkMs = 0;
#ifndef DOCK_CTRL_ACK_PULSE_MS
#define DOCK_CTRL_ACK_PULSE_MS 500u
#endif

void dockControllerNotifyApproachSendOk(void) { s_lastApproachSendOkMs = millis(); }
void dockControllerNotifyChargeSendOk(void) { s_lastChargeSendOkMs = millis(); }

uint8_t dockControllerGetLiveAckMask(void) {
  uint32_t t = millis();
  uint8_t m = 0;
  if (s_lastApproachSendOkMs && (t - s_lastApproachSendOkMs) < DOCK_CTRL_ACK_PULSE_MS) {
    m |= (uint8_t)WALLE_ACK_APPROACH_STAGE;
  }
  if (s_lastChargeSendOkMs && (t - s_lastChargeSendOkMs) < DOCK_CTRL_ACK_PULSE_MS) {
    m |= (uint8_t)WALLE_ACK_CHARGE_REQUEST;
  }
  return m;
}

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

bool dockControllerSendRequestCharge(uint32_t dock_id) {
  if (!ensureBroadcastPeer()) return false;
  DockCommandPacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = DOCK_CMD_REQUEST_CHARGE;
  esp_err_t r = esp_now_send(s_broadcast_mac, (uint8_t*)&pkt, sizeof(pkt));
  if (r == ESP_OK) dockControllerNotifyChargeSendOk();
  return (r == ESP_OK);
}

bool dockControllerSendApproachStage(uint8_t stage, uint32_t dock_id) {
  if (!ensureBroadcastPeer()) return false;
  DockApproachStagePacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = DOCK_CMD_APPROACH_STAGE;
  pkt.stage = (stage <= APPROACH_DOCKED) ? stage : APPROACH_FAR;
  esp_err_t r = esp_now_send(s_broadcast_mac, (uint8_t *)&pkt, sizeof(pkt));
  if (r == ESP_OK) dockControllerNotifyApproachSendOk();
  return (r == ESP_OK);
}

bool dockControllerSendDockingArm(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_DOCKING_ARM);
}

bool dockControllerSendDockingDisarm(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_DOCKING_DISARM);
}
