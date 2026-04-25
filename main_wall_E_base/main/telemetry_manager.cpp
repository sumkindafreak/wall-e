#include "telemetry_manager.h"
#include "node_health_registry.h"
#include "node_health_protocol.h"
#include "espnow_receiver.h"
#include "walle_link_packet.h"
#include "audio_ui_telemetry.h"
#include "audio_esp_status.h"
#include "shared_voicebox_manager.h"
#include "relationship_manager.h"
#include "autonomy_manager.h"
#include "battery_monitor.h"
#include "eve_uart_bridge.h"
#include "eve_target_assist.h"

String telemetryManagerGetJSON(void) {
  const WalleBondState* b = relationshipGetState();
  const BatteryData& bat = batteryGetData();

  String j = "{\"schema\":2";
  j += ",\"uptime_ms\":"; j += String(millis());
  j += ",\"nodes\":"; j += nodeHealthGetJSON();

  j += ",\"voicebox_mode\":\""; j += sharedVoiceboxModeName(); j += "\"";
  j += ",\"voicebox_shared\":"; j += sharedVoiceboxIsShared() ? "true" : "false";
  j += ",\"eve_uart\":"; j += eveUartBridgeIsLinkUp() ? "true" : "false";
  j += ",\"bond_strength\":"; j += (unsigned)relationshipGetBondStrength();
  j += ",\"bond_trust\":"; j += (unsigned)b->trust_level;
  j += ",\"bond_comfort\":"; j += (unsigned)b->comfort_level;
  j += ",\"bond_curious\":"; j += (unsigned)b->curiosity_level;
  j += ",\"bond_shared_docks\":"; j += (unsigned)b->shared_dock_events;
  j += ",\"behavior\":\""; j += autonomyManagerGetStateName(); j += "\"";
  j += ",\"eve_assist\":"; j += eveTargetAssistGetStatusJSON();

  j += ",\"audio_ui_valid\":"; j += audioUiTelemValid() ? "true" : "false";
  j += ",\"btn_mode\":"; j += (unsigned)audioUiTelemGetBtnMode();
  j += ",\"menu_page\":"; j += (unsigned)audioUiTelemGetMenuPage();
  j += ",\"menu_combo_pct\":"; j += (unsigned)audioUiTelemGetComboPct();
  j += ",\"last_ui_event\":"; j += (unsigned)audioUiTelemGetLastEvent();

  j += ",\"battery_pct\":"; j += bat.valid ? String(bat.percent) : String("-1");
  j += ",\"dock_ir\":"; j += audioEspStatusValid() ? String((int)audioEspStatusGetDockIr()) : String("-1");

  {
    uint16_t seq = 0;
    uint8_t ack = 0;
    espnowReceiverGetCydCommsForApi(&seq, &ack);
    j += ",\"cyd_comms\":{";
    j += "\"last_control_seq\":"; j += (uint32_t)seq;
    j += ",\"ack_bits\":"; j += (unsigned)ack;
    j += ",\"ack_estop\":"; j += (ack & WALLE_ACK_ESTOP) ? "true" : "false";
    j += ",\"ack_motion_policy\":"; j += (ack & WALLE_ACK_MOTION_POLICY) ? "true" : "false";
    j += ",\"ack_charge\":"; j += (ack & WALLE_ACK_CHARGE_REQUEST) ? "true" : "false";
    j += ",\"ack_approach\":"; j += (ack & WALLE_ACK_APPROACH_STAGE) ? "true" : "false";
    j += "}";
  }

  j += "}";
  return j;
}
