#include "autonomy_manager.h"
#include "node_health_protocol.h"
#include "battery_monitor.h"
#include "eve_uart_bridge.h"
#include "node_health_registry.h"
#include "audio_ui_telemetry.h"
#include "emotion_engine.h"
#include "autonomous_docking.h"
#include "dock_homing.h"
#include "dock_config.h"
#include "memory_manager.h"
#include "memory_protocol.h"

static WalleBehaviorState s_st = BEH_IDLE;
static uint32_t s_lastLogMs = 0;

static const char* nameFor(WalleBehaviorState s) {
  switch (s) {
    case BEH_IDLE: return "IDLE";
    case BEH_ATTENTION: return "ATTENTION";
    case BEH_CURIOUS: return "CURIOUS";
    case BEH_COMPANION: return "COMPANION";
    case BEH_ALERT: return "ALERT";
    case BEH_LOW_BATTERY: return "LOW_BATTERY";
    case BEH_DOCKING: return "DOCKING";
    case BEH_SLEEP: return "SLEEP";
    case BEH_SEARCHING_FOR_EVE: return "SEARCHING_FOR_EVE";
    case BEH_CELEBRATION: return "CELEBRATION";
    default: return "UNKNOWN";
  }
}

void autonomyManagerInit(void) {
  s_st = BEH_IDLE;
  s_lastLogMs = 0;
  Serial.println(F("[AUTO] behavior orchestrator init"));
}

WalleBehaviorState autonomyManagerGetState(void) { return s_st; }
const char* autonomyManagerGetStateName(void) { return nameFor(s_st); }

void autonomyManagerTick(uint32_t nowMillis) {
  const BatteryData& bat = batteryGetData();
  bool lowBat = bat.valid && bat.percent >= 0 && bat.percent < 20;
  bool dockBusy = false;
#if USE_AUTONOMOUS_DOCKING
  dockBusy = autonomousDockingIsActive();
#else
  dockBusy = dockHomingIsActive();
#endif
  bool eveUp = eveUartBridgeIsLinkUp();
  bool audioUi = audioUiTelemValid();
  bool visionUp = nodeHealthIsOnline(WALLE_NODE_VISION);

  WalleBehaviorState next = BEH_IDLE;
  if (lowBat)
    next = BEH_LOW_BATTERY;
  else if (dockBusy)
    next = BEH_DOCKING;
  else if (!eveUp && nodeHealthIsOnline(WALLE_NODE_AUDIO))
    next = BEH_SEARCHING_FOR_EVE;
  else if (eveUp && audioUi)
    next = BEH_COMPANION;
  else if (visionUp)
    next = BEH_ATTENTION;
  else
    next = BEH_IDLE;

  if (next != s_st) {
    s_st = next;
    Serial.printf("[AUTO] behavior -> %s\n", nameFor(s_st));
    if (s_st == BEH_LOW_BATTERY) {
      emotionTriggerBatteryLow();
      memoryManagerLog(WALLE_MEM_EV_BATTERY, "low_behavior");
    } else if (s_st == BEH_COMPANION) {
      emotionTransitionTo(EMOTION_CALM, 0.6f, 2000);
    }
  }

  if (nowMillis - s_lastLogMs > 15000u) {
    s_lastLogMs = nowMillis;
    Serial.printf("[AUTO] tick state=%s eve=%d audio_ui=%d\n", nameFor(s_st), eveUp ? 1 : 0,
                  audioUi ? 1 : 0);
  }
}
