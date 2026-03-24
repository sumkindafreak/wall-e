#include "walle_emotion_pose_bridge.h"
#include "walle_emotion_pose.h"
#include "battery_monitor.h"
#include "node_health_registry.h"
#include "node_health_protocol.h"
#include "autonomous_docking.h"
#include "dock_config.h"
#include "vision_behaviour.h"

void walleEmotionPoseBridgeInit(void) {
  walleEmotionPoseInit();
}

void walleEmotionPoseBridgeTick(void) {
  WalleEmotionInputs in = {};
  const BatteryData& b = batteryGetData();
  in.batteryPercent = (b.valid && b.percent >= 0) ? (float)b.percent : -1.0f;
  in.brainLinkOk = nodeHealthIsOnline(WALLE_NODE_MASTER) ? 1u : 0u;
  in.visionOnline = nodeHealthIsOnline(WALLE_NODE_VISION) ? 1u : 0u;
  in.loudSound = 0u;
  in.humanDetected = visionBehaviourIsEngaged() ? 1u : 0u;

  bool docked = false;
#if USE_AUTONOMOUS_DOCKING
  DockState ds = autonomousDockingGetState();
  docked = (ds == DOCK_STATE_DOCKED || ds == DOCK_STATE_CHARGING);
#endif
  if (!docked) {
    uint16_t fl = nodeHealthGetFlags(WALLE_NODE_DOCK);
    if (fl & WALLE_NODE_FLAG_DOCKED) docked = true;
  }
  in.isDocked = docked ? 1u : 0u;

  walleEmotionPoseUpdateFromInputs(&in);
}
