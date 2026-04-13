#include "relationship_manager.h"
#include "eve_uart_bridge.h"
#include "memory_manager.h"
#include "memory_protocol.h"
#include <Preferences.h>
#include <string.h>

static WalleBondState s_bond;
static bool s_eveWasUp = false;

static void loadPrefs(void) {
  Preferences p;
  if (!p.begin(WALLE_REL_PREFS_NS, true)) return;
  s_bond.attachment_count = p.getUInt("att", 0);
  s_bond.cooperative_moments = p.getUInt("coop", 0);
  s_bond.shared_dock_events = p.getUInt("dock", 0);
  s_bond.comfort_level = p.getUChar("comfort", 40);
  s_bond.trust_level = p.getUChar("trust", 40);
  s_bond.curiosity_level = p.getUChar("curious", 50);
  s_bond.last_interaction_type = p.getUChar("last_ix", 0);
  p.end();
}

static void savePrefs(void) {
  Preferences p;
  if (!p.begin(WALLE_REL_PREFS_NS, false)) return;
  p.putUInt("att", s_bond.attachment_count);
  p.putUInt("coop", s_bond.cooperative_moments);
  p.putUInt("dock", s_bond.shared_dock_events);
  p.putUChar("comfort", s_bond.comfort_level);
  p.putUChar("trust", s_bond.trust_level);
  p.putUChar("curious", s_bond.curiosity_level);
  p.putUChar("last_ix", s_bond.last_interaction_type);
  p.end();
}

static void recomputeBond(void) {
  uint32_t score = (uint32_t)s_bond.comfort_level + s_bond.trust_level + s_bond.curiosity_level;
  score += (s_bond.cooperative_moments > 100) ? 30u : (s_bond.cooperative_moments / 4);
  score += (s_bond.attachment_count > 50) ? 20u : (s_bond.attachment_count / 3);
  if (score > 255u) score = 255u;
  s_bond.bond_strength = (uint8_t)score;
}

void relationshipInit(void) {
  memset(&s_bond, 0, sizeof(s_bond));
  loadPrefs();
  recomputeBond();
  s_eveWasUp = false;
  Serial.println(F("[BOND] relationship manager init"));
}

const WalleBondState* relationshipGetState(void) { return &s_bond; }
uint8_t relationshipGetBondStrength(void) { return s_bond.bond_strength; }

void relationshipOnDockSharedEvent(void) {
  s_bond.shared_dock_events++;
  s_bond.cooperative_moments++;
  s_bond.last_interaction_type = WALLE_LAST_IX_DOCK;
  recomputeBond();
  savePrefs();
  memoryManagerLog(WALLE_MEM_EV_DOCK, "shared_dock");
}

void relationshipTick(uint32_t nowMillis) {
  bool eve = eveUartBridgeIsLinkUp();
  if (eve && !s_eveWasUp) {
    s_bond.attachment_count++;
    s_bond.last_seen_together_ms_epoch = nowMillis;
    s_bond.cooperative_moments++;
    s_bond.last_interaction_type = WALLE_LAST_IX_GREET;
    recomputeBond();
    savePrefs();
    Serial.println(F("[BOND] EVE online — bond event"));
    memoryManagerLog(WALLE_MEM_EV_EVE_ATTACH, "eve_uart_up");
  }
  if (!eve && s_eveWasUp) {
    Serial.println(F("[DISCOVERY] EVE UART offline"));
    memoryManagerLog(WALLE_MEM_EV_EVE_DETACH, "eve_uart_lost");
  }
  s_eveWasUp = eve;
}
