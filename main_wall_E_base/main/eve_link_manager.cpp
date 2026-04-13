#include "eve_link_manager.h"
#include "eve_uart_bridge.h"

void eveLinkManagerInit(void) {
  Serial.println(F("[EVE] link manager (UART facade)"));
}

void eveLinkManagerTick(uint32_t nowMillis) {
  (void)nowMillis;
}

bool eveLinkIsEveUartUp(void) {
  return eveUartBridgeIsLinkUp();
}
