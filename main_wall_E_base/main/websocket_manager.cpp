#include "websocket_manager.h"
#include "telemetry_manager.h"
#include <WebSocketsServer.h>

static WebSocketsServer s_ws(81);
static uint32_t s_lastBroadcastMs = 0;

#define WS_TELEM_PERIOD_MS 500u

static void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  (void)payload;
  (void)length;
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[WS] client #%u connected\n", num);
      {
        String j = telemetryManagerGetJSON();
        s_ws.sendTXT(num, j);
      }
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[WS] client #%u disconnected\n", num);
      break;
    default:
      break;
  }
}

void websocketManagerBegin(void) {
  s_ws.begin();
  s_ws.onEvent(wsEvent);
  Serial.println(F("[WS] WebSocket server on port 81 (path /)"));
}

void websocketManagerLoop(void) {
  s_ws.loop();
  uint32_t now = millis();
  if (now - s_lastBroadcastMs >= WS_TELEM_PERIOD_MS) {
    s_lastBroadcastMs = now;
    if (s_ws.connectedClients() > 0) {
      String j = telemetryManagerGetJSON();
      s_ws.broadcastTXT(j);
    }
  }
}
