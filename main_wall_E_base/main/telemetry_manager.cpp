#include "telemetry_manager.h"
#include "node_health_registry.h"
#include "node_health_protocol.h"
#include "audio_ui_telemetry.h"
#include "audio_esp_status.h"
#include "shared_voicebox_manager.h"
#include "relationship_manager.h"
#include "autonomy_manager.h"
#include "battery_monitor.h"
#include "eve_uart_bridge.h"

String telemetryManagerGetJSON(void) {
  const WalleBondState* b = relationshipGetState();
  const BatteryData& bat = batteryGetData();

  String j = "{\"schema\":1";
  j += ",\"uptime_ms\":"; j += String(millis());
  j += ",\"nodes\":"; j += nodeHealthGetJSON();

  j += ",\"voicebox_mode\":\""; j += sharedVoiceboxModeName(); j += "\"";
  j += ",\"voicebox_shared\":"; j += sharedVoiceboxIsShared() ? "true" : "false";
  j += ",\"eve_uart\":"; j += eveUartBridgeIsLinkUp() ? "true" : "false";
  j += ",\"bond_strength\":"; j += (unsigned)relationshipGetBondStrength();
  j += ",\"bond_trust\":"; j += (unsigned)b->trust_level;
  j += ",\"behavior\":\""; j += autonomyManagerGetStateName(); j += "\"";

  j += ",\"audio_ui_valid\":"; j += audioUiTelemValid() ? "true" : "false";
  j += ",\"btn_mode\":"; j += (unsigned)audioUiTelemGetBtnMode();
  j += ",\"menu_page\":"; j += (unsigned)audioUiTelemGetMenuPage();
  j += ",\"menu_combo_pct\":"; j += (unsigned)audioUiTelemGetComboPct();
  j += ",\"last_ui_event\":"; j += (unsigned)audioUiTelemGetLastEvent();

  j += ",\"battery_pct\":"; j += bat.valid ? String(bat.percent) : String("-1");
  j += ",\"dock_ir\":"; j += audioEspStatusValid() ? String((int)audioEspStatusGetDockIr()) : String("-1");

  j += "}";
  return j;
}
