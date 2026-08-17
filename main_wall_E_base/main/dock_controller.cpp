// ============================================================
// WALL-E Dock Controller - radio commands to dock
// ============================================================

#include "dock_controller.h"
#include "dock_protocol.h"
#include "radio_transport.h"
#include <Arduino.h>
#include <cstring>

#define DOCK_CMD_MAGIC           0x434D444B
#define DOCK_CMD_FORCE_OFF       1
#define DOCK_CMD_RESET           2
#define DOCK_CMD_WIFI_CONFIG     3
#define DOCK_CMD_TIME            4
#define DOCK_CMD_REQUEST_CHARGE  5
#define DOCK_CMD_APPROACH_STAGE  6
#define DOCK_CMD_DOCKING_ARM     7
#define DOCK_CMD_DOCKING_DISARM  8

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

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t dock_id;
  uint8_t  cmd;
  uint8_t  pad[3];
  uint32_t unix_time;
} DockTimePacket_t;

static bool sendBroadcast(const void* packet, size_t length) {
  return packet && length > 0 && radioTransportBroadcast(packet, length);
}

static bool sendCmd(uint32_t dock_id, uint8_t cmd) {
  DockCommandPacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = cmd;
  return sendBroadcast(&pkt, sizeof(pkt));
}

void dockControllerInit(void) {
  (void)radioTransportInit();
}

bool dockControllerSendWifiConfig(const char* ssid, const char* pass, uint32_t dock_id) {
  if (!ssid || strlen(ssid) == 0 || strlen(ssid) > 32) return false;

  DockWifiConfigPacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = DOCK_CMD_WIFI_CONFIG;
  strncpy(pkt.ssid, ssid, 32);
  pkt.ssid[32] = '\0';
  if (pass) strncpy(pkt.pass, pass, 64);
  pkt.pass[64] = '\0';
  return sendBroadcast(&pkt, sizeof(pkt));
}

bool dockControllerSendTime(uint32_t unix_time, uint32_t dock_id) {
  DockTimePacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = DOCK_CMD_TIME;
  pkt.unix_time = unix_time;
  return sendBroadcast(&pkt, sizeof(pkt));
}

bool dockControllerSendForceOff(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_FORCE_OFF);
}

bool dockControllerSendReset(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_RESET);
}

bool dockControllerSendRequestCharge(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_REQUEST_CHARGE);
}

bool dockControllerSendApproachStage(uint8_t stage, uint32_t dock_id) {
  DockApproachStagePacket_t pkt = {};
  pkt.magic = DOCK_CMD_MAGIC;
  pkt.dock_id = dock_id;
  pkt.cmd = DOCK_CMD_APPROACH_STAGE;
  pkt.stage = (stage <= APPROACH_DOCKED) ? stage : APPROACH_FAR;
  return sendBroadcast(&pkt, sizeof(pkt));
}

bool dockControllerSendDockingArm(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_DOCKING_ARM);
}

bool dockControllerSendDockingDisarm(uint32_t dock_id) {
  return sendCmd(dock_id, DOCK_CMD_DOCKING_DISARM);
}
