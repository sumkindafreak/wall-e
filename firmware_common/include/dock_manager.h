#pragma once

/*
 * EVE smart dock environment-node manager.
 *
 * This module does not touch pins. Existing dock firmware supplies callbacks for
 * charging, lighting, and UART send functions. The manager tracks state, emits
 * status, and decides when memory sync should begin.
 */

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

#include "uart_protocol.h"

enum DockNodeState : uint8_t {
  DOCK_NODE_IDLE = 0,
  DOCK_NODE_EVE_DOCKED = 1,
  DOCK_NODE_SYNCING = 2,
  DOCK_NODE_CHARGING = 3,
  DOCK_NODE_INTERACTIVE = 4,
  DOCK_NODE_FAULT = 5
};

enum DockLightMode : uint8_t {
  DOCK_LIGHT_OFF = 0,
  DOCK_LIGHT_IDLE = 1,
  DOCK_LIGHT_DOCKED = 2,
  DOCK_LIGHT_SYNCING = 3,
  DOCK_LIGHT_CHARGING = 4,
  DOCK_LIGHT_INTERACTIVE = 5,
  DOCK_LIGHT_FAULT = 6
};

typedef bool (*DockSetChargingCallback)(bool enable);
typedef void (*DockSetLightCallback)(DockLightMode mode);
typedef bool (*DockSendJsonCallback)(uint8_t type, const char* json);

struct DockManagerCallbacks {
  DockSetChargingCallback setCharging = nullptr;
  DockSetLightCallback setLight = nullptr;
  DockSendJsonCallback sendJson = nullptr;
};

struct DockEnvironmentReading {
  int16_t ambientTempC_x10 = 0;
  uint16_t ambientLight = 0;
  uint16_t supplyMv = 0;
  bool powerGood = false;
};

class DockManager {
 public:
  void begin(const DockManagerCallbacks& callbacks) {
    callbacks_ = callbacks;
    state_ = DOCK_NODE_IDLE;
    eveDocked_ = false;
    charging_ = false;
    syncRequested_ = false;
    syncActive_ = false;
    session_ = 0;
    lastEveRxMs_ = 0;
    lastStatusTxMs_ = 0;
    env_ = DockEnvironmentReading();
    applyLighting();
  }

  DockNodeState state() const { return state_; }
  bool eveDocked() const { return eveDocked_; }
  bool charging() const { return charging_; }
  bool syncRequested() const { return syncRequested_; }
  bool syncActive() const { return syncActive_; }
  uint32_t session() const { return session_; }

  void setEnvironment(const DockEnvironmentReading& env) { env_ = env; }

  void onEveHello(uint32_t nowMs, uint32_t session) {
    eveDocked_ = true;
    lastEveRxMs_ = nowMs;
    session_ = session ? session : makeSession(nowMs);
    syncRequested_ = true;
    syncActive_ = false;
    setState(DOCK_NODE_EVE_DOCKED);
    sendIdentify();
  }

  void onEveStatus(uint32_t nowMs, bool wantsInteraction) {
    eveDocked_ = true;
    lastEveRxMs_ = nowMs;
    if (syncActive_) {
      setState(DOCK_NODE_SYNCING);
    } else if (wantsInteraction) {
      setState(DOCK_NODE_INTERACTIVE);
    } else if (charging_) {
      setState(DOCK_NODE_CHARGING);
    } else {
      setState(DOCK_NODE_EVE_DOCKED);
    }
  }

  void onSyncBegin(uint32_t nowMs) {
    lastEveRxMs_ = nowMs;
    syncRequested_ = false;
    syncActive_ = true;
    setState(DOCK_NODE_SYNCING);
  }

  void onSyncComplete(uint32_t nowMs) {
    lastEveRxMs_ = nowMs;
    syncActive_ = false;
    syncRequested_ = false;
    setState(charging_ ? DOCK_NODE_CHARGING : DOCK_NODE_EVE_DOCKED);
  }

  void setCharging(bool enable) {
    bool ok = true;
    if (callbacks_.setCharging) ok = callbacks_.setCharging(enable);
    charging_ = ok && enable;
    if (eveDocked_) setState(charging_ ? DOCK_NODE_CHARGING : DOCK_NODE_EVE_DOCKED);
  }

  void tick(uint32_t nowMs) {
    if (eveDocked_ && lastEveRxMs_ != 0 && (uint32_t)(nowMs - lastEveRxMs_) > 25000u) {
      failSafeDisconnect();
    }
    if ((uint32_t)(nowMs - lastStatusTxMs_) > 1000u) {
      lastStatusTxMs_ = nowMs;
      sendStatus(nowMs);
    }
  }

  void serializeStatusJson(char* out, size_t outLen, uint32_t nowMs) const {
    if (!out || outLen == 0) return;
    snprintf(out, outLen,
             "{\"src\":\"DOCK\",\"type\":\"status\",\"state\":\"%s\",\"session\":%lu,"
             "\"eve_docked\":%s,\"charging\":%s,\"sync_requested\":%s,\"sync_active\":%s,"
             "\"last_eve_age_ms\":%lu,\"power_good\":%s,\"supply_mv\":%u,"
             "\"ambient_temp_c_x10\":%d,\"ambient_light\":%u}",
             stateName(state_), (unsigned long)session_, eveDocked_ ? "true" : "false",
             charging_ ? "true" : "false", syncRequested_ ? "true" : "false",
             syncActive_ ? "true" : "false",
             (unsigned long)(lastEveRxMs_ ? (uint32_t)(nowMs - lastEveRxMs_) : 0u),
             env_.powerGood ? "true" : "false", (unsigned)env_.supplyMv,
             (int)env_.ambientTempC_x10, (unsigned)env_.ambientLight);
  }

  static const char* stateName(DockNodeState state) {
    switch (state) {
      case DOCK_NODE_IDLE: return "IDLE";
      case DOCK_NODE_EVE_DOCKED: return "EVE_DOCKED";
      case DOCK_NODE_SYNCING: return "SYNCING";
      case DOCK_NODE_CHARGING: return "CHARGING";
      case DOCK_NODE_INTERACTIVE: return "INTERACTIVE";
      case DOCK_NODE_FAULT: return "FAULT";
      default: return "UNKNOWN";
    }
  }

 private:
  uint32_t makeSession(uint32_t nowMs) const {
    uint32_t s = nowMs ^ 0xD0C0E5u;
    return s == 0 ? 1u : s;
  }

  void setState(DockNodeState next) {
    if (state_ == next) return;
    state_ = next;
    applyLighting();
  }

  void applyLighting() {
    if (!callbacks_.setLight) return;
    switch (state_) {
      case DOCK_NODE_IDLE: callbacks_.setLight(DOCK_LIGHT_IDLE); break;
      case DOCK_NODE_EVE_DOCKED: callbacks_.setLight(DOCK_LIGHT_DOCKED); break;
      case DOCK_NODE_SYNCING: callbacks_.setLight(DOCK_LIGHT_SYNCING); break;
      case DOCK_NODE_CHARGING: callbacks_.setLight(DOCK_LIGHT_CHARGING); break;
      case DOCK_NODE_INTERACTIVE: callbacks_.setLight(DOCK_LIGHT_INTERACTIVE); break;
      case DOCK_NODE_FAULT: callbacks_.setLight(DOCK_LIGHT_FAULT); break;
    }
  }

  void sendIdentify() {
    if (!callbacks_.sendJson) return;
    char json[128];
    uartBusMakeHelloJson(json, sizeof(json), UART_BUS_DEVICE_DOCK, UART_BUS_ROLE_ENVIRONMENT_NODE,
                         session_, "eve_smart_dock");
    callbacks_.sendJson(UART_BUS_IDENTIFY, json);
  }

  void sendStatus(uint32_t nowMs) {
    if (!callbacks_.sendJson) return;
    char json[260];
    serializeStatusJson(json, sizeof(json), nowMs);
    callbacks_.sendJson(UART_BUS_STATUS, json);
  }

  void failSafeDisconnect() {
    if (callbacks_.setCharging) callbacks_.setCharging(false);
    eveDocked_ = false;
    charging_ = false;
    syncRequested_ = false;
    syncActive_ = false;
    session_ = 0;
    lastEveRxMs_ = 0;
    setState(DOCK_NODE_IDLE);
  }

  DockManagerCallbacks callbacks_;
  DockEnvironmentReading env_;
  DockNodeState state_ = DOCK_NODE_IDLE;
  bool eveDocked_ = false;
  bool charging_ = false;
  bool syncRequested_ = false;
  bool syncActive_ = false;
  uint32_t session_ = 0;
  uint32_t lastEveRxMs_ = 0;
  uint32_t lastStatusTxMs_ = 0;
};

