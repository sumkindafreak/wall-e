/**
 * WALL-E base: serial port for the EVE hand link.
 * Frames are decoded with the shared EveFrameDecoder (CRC + protocol version checked); only valid
 * frames reach eve_peer_link, which runs the handshake, heartbeats and link supervision.
 */
#include "eve_uart_bridge.h"

#include <HardwareSerial.h>
#include <esp_random.h>
#include <string.h>

#include "../../firmware_common/include/eve_link_protocol.h"
#include "eve_peer_link.h"
#include "eve_target_assist.h"

#ifndef EVE_BRIDGE_ENABLE
#define EVE_BRIDGE_ENABLE 1
#endif

static HardwareSerial EveSerial(2);
static EveFrameDecoder s_decoder;
static uint8_t s_frame[EVE_MAX_PAYLOAD + EVE_FRAME_OVERHEAD];

static bool bridgeSend(uint8_t type, uint8_t seq, const char* json) {
#if !EVE_BRIDGE_ENABLE
  (void)type;
  (void)seq;
  (void)json;
  return false;
#else
  const size_t plen = json ? strlen(json) : 0;
  if (plen > EVE_MAX_PAYLOAD) return false;
  const size_t n = eveLinkEncode(type, seq, (const uint8_t*)json, (uint16_t)plen, s_frame, sizeof(s_frame));
  if (n == 0) return false;
  return EveSerial.write(s_frame, n) == n;
#endif
}

static void bridgeLog(const char* line) { Serial.println(line); }

static uint32_t bridgeRandom(void) { return esp_random(); }

void eveUartBridgeInit(void) {
#if EVE_BRIDGE_ENABLE
  /* Buffer sizes must be set before begin() on Arduino-ESP32 3.x. */
  EveSerial.setRxBufferSize(1024);
  EveSerial.setTxBufferSize(1024);
  EveSerial.begin(EVE_BRIDGE_BAUD, SERIAL_8N1, EVE_BRIDGE_UART_RX, EVE_BRIDGE_UART_TX);
  s_decoder.reset();
  static const EvePeerHooks hooks = {bridgeSend, bridgeLog, eveTargetAssistIngestJson, bridgeRandom};
  evePeerInit(&hooks, millis());
  Serial.printf("[EVE bridge] UART2 RX=%d TX=%d baud=%lu\n", EVE_BRIDGE_UART_RX, EVE_BRIDGE_UART_TX,
                (unsigned long)EVE_BRIDGE_BAUD);
#endif
}

void eveUartBridgePoll(void) {
#if EVE_BRIDGE_ENABLE
  int budget = 512; /* bounded per loop pass so WALL-E's own control loop stays responsive */
  while (budget-- > 0 && EveSerial.available() > 0) {
    if (s_decoder.feed((uint8_t)EveSerial.read())) {
      evePeerOnFrame(s_decoder.type(), s_decoder.payload(), s_decoder.len(), s_decoder.seq(), millis());
    }
  }
  evePeerTick(millis());
#endif
}

String eveUartBridgeGetJSON(void) {
  static char buf[2048];
  const EveFrameDecoder::Stats& st = s_decoder.stats();
  evePeerStatusJson(buf, sizeof(buf), millis(), st.crcErr, st.versionErr, st.lengthErr);
  return String(buf);
}

bool eveUartBridgeIsLinkUp(void) { return evePeerLinked(); }

bool eveUartBridgeEveStopped(void) {
  const EvePeerEveStatus* e = evePeerEveStatus();
  return evePeerLinked() && e->valid && e->stop;
}

bool eveUartBridgeSendPlaySound(uint8_t track) { return evePeerSendSound(track); }

bool eveUartBridgeSendCydServo(uint8_t headPanDeg, uint8_t rightArmDeg) {
  return evePeerSendMove((int)headPanDeg, (int)rightArmDeg);
}

bool eveUartBridgeSendBusCommand(const char* jsonUtf8) { return evePeerSendBusCommand(jsonUtf8); }

bool eveUartBridgeSendCompanionToEve(const char* jsonUtf8) { return eveUartBridgeSendBusCommand(jsonUtf8); }

String eveUartBridgeGetMonitorJSON(void) {
  String j = "{\"ok\":true,\"bus\":\"uart_only\",\"peer\":";
  j += eveUartBridgeIsLinkUp() ? "\"EVE\"" : "null";
  j += ",\"bridge\":";
  j += eveUartBridgeGetJSON();
  j += "}";
  return j;
}

bool eveUartBridgeSendWallEAck(uint32_t session) {
  char ack[96];
  snprintf(ack, sizeof(ack), "{\"session\":%lu,\"peer\":\"wall_e\",\"hb\":%lu}", (unsigned long)session,
           (unsigned long)EVE_LINK_HEARTBEAT_PERIOD_MS);
  return bridgeSend(MSG_WALL_E_ACK, 0, ack);
}

bool eveUartBridgeSendStop(const char* src) { return evePeerSendStop(src); }

bool eveUartBridgeSendClearStop(void) { return evePeerSendClearStop(); }

bool eveUartBridgeSendModeByName(const char* name) {
  if (!name) return false;
  if (!strcmp(name, "escort")) return evePeerSendMode(MSG_MODE_ESCORT);
  if (!strcmp(name, "dock")) return evePeerSendMode(MSG_MODE_DOCK);
  if (!strcmp(name, "idle")) return evePeerSendMode(MSG_MODE_IDLE);
  if (!strcmp(name, "interact")) return evePeerSendMode(MSG_MODE_INTERACT);
  if (!strcmp(name, "standby")) return evePeerSendMode(MSG_MODE_STANDBY);
  if (!strcmp(name, "attach")) return evePeerSendMode(MSG_ATTACH_CONFIRMED);
  return false;
}

bool eveUartBridgeSendRoutine(const char* name) { return evePeerSendRoutine(name); }

bool eveUartBridgeSendEyes(const char* name, uint16_t holdMs) { return evePeerSendEyes(name, holdMs); }

bool eveUartBridgeSendGlow(const char* name) { return evePeerSendGlow(name); }

bool eveUartBridgeSendReset(void) { return evePeerSendResetState(); }
