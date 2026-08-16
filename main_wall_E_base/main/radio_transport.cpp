#include "radio_transport.h"
#include "../../protocols/walle_radio_bridge_protocol.h"

#include <Arduino.h>
#include <esp_arduino_version.h>
#include <cstring>

static WalleRadioReceiveCallback s_receiveCallback = nullptr;
static uint32_t s_lastRxMs = 0;
static uint32_t s_rxCount = 0;
static uint32_t s_txCount = 0;
static uint32_t s_txFailures = 0;
static uint8_t s_channel = WALLE_RADIO_CHANNEL;

void radioTransportSetReceiveCallback(WalleRadioReceiveCallback callback) {
  s_receiveCallback = callback;
}

uint8_t radioTransportGetChannel(void) { return s_channel; }
uint32_t radioTransportGetLastRxMs(void) { return s_lastRxMs; }
uint32_t radioTransportGetRxCount(void) { return s_rxCount; }
uint32_t radioTransportGetTxCount(void) { return s_txCount; }
uint32_t radioTransportGetTxFailureCount(void) { return s_txFailures; }

#if defined(CONFIG_IDF_TARGET_ESP32P4)

// ============================================================
// ESP32-P4: UART1 radio gateway transport
// UART0 = Waveshare CH343 debug/programming, UART2 = GPS.
// ============================================================

static HardwareSerial s_radioUart(1);
static bool s_started = false;
static bool s_gatewayReady = false;
static uint32_t s_lastGatewayStatusMs = 0;
static uint32_t s_lastControlTxMs = 0;
static uint16_t s_sequence = 0;

static uint8_t s_frameBuffer[sizeof(WalleRadioBridgeHeader) +
                             WALLE_RADIO_BRIDGE_MAX_PAYLOAD + 2];
static size_t s_framePos = 0;
static size_t s_frameTarget = 0;

enum ParserState : uint8_t {
  PARSER_WAIT_SOF_LO = 0,
  PARSER_WAIT_SOF_HI,
  PARSER_READ_HEADER,
  PARSER_READ_REST
};
static ParserState s_parserState = PARSER_WAIT_SOF_LO;

static bool isBroadcastMac(const uint8_t mac[6]) {
  if (!mac) return false;
  for (uint8_t i = 0; i < 6; ++i) {
    if (mac[i] != 0xFFu) return false;
  }
  return true;
}

static bool sendBridgeFrame(uint8_t type,
                            const uint8_t* payload,
                            uint16_t payloadLength) {
  if (!s_started || payloadLength > WALLE_RADIO_BRIDGE_MAX_PAYLOAD) return false;

  WalleRadioBridgeHeader header = {};
  header.sof = WALLE_RADIO_BRIDGE_SOF;
  header.version = WALLE_RADIO_BRIDGE_VERSION;
  header.type = type;
  header.sequence = ++s_sequence;
  header.payloadLength = payloadLength;

  uint16_t crc = walleRadioCrc16(
      reinterpret_cast<const uint8_t*>(&header), sizeof(header));
  if (payloadLength && payload) crc = walleRadioCrc16(payload, payloadLength, crc);

  size_t written = s_radioUart.write(
      reinterpret_cast<const uint8_t*>(&header), sizeof(header));
  if (payloadLength && payload) written += s_radioUart.write(payload, payloadLength);

  const uint8_t crcBytes[2] = {
      (uint8_t)(crc & 0xFFu),
      (uint8_t)(crc >> 8)
  };
  written += s_radioUart.write(crcBytes, sizeof(crcBytes));
  return written == sizeof(header) + payloadLength + sizeof(crcBytes);
}

static void sendChannelRequest(void) {
  const uint8_t channel = WALLE_RADIO_CHANNEL;
  (void)sendBridgeFrame(WALLE_RADIO_MSG_SET_CHANNEL, &channel, sizeof(channel));
}

static void parserReset(void) {
  s_parserState = PARSER_WAIT_SOF_LO;
  s_framePos = 0;
  s_frameTarget = 0;
}

static void handleCompleteFrame(void) {
  if (s_frameTarget < sizeof(WalleRadioBridgeHeader) + 2) {
    parserReset();
    return;
  }

  const WalleRadioBridgeHeader* header =
      reinterpret_cast<const WalleRadioBridgeHeader*>(s_frameBuffer);
  if (header->sof != WALLE_RADIO_BRIDGE_SOF ||
      header->version != WALLE_RADIO_BRIDGE_VERSION ||
      header->payloadLength > WALLE_RADIO_BRIDGE_MAX_PAYLOAD) {
    parserReset();
    return;
  }

  const size_t bodyLength = sizeof(WalleRadioBridgeHeader) + header->payloadLength;
  const uint16_t expectedCrc = walleRadioCrc16(s_frameBuffer, bodyLength);
  const uint16_t receivedCrc =
      (uint16_t)s_frameBuffer[bodyLength] |
      ((uint16_t)s_frameBuffer[bodyLength + 1] << 8);
  if (expectedCrc != receivedCrc) {
    Serial.println(F("[Radio/P4] CRC error from gateway"));
    parserReset();
    return;
  }

  const uint8_t* payload = s_frameBuffer + sizeof(WalleRadioBridgeHeader);
  switch (header->type) {
    case WALLE_RADIO_MSG_RX_PACKET: {
      if (header->payloadLength < sizeof(WalleRadioRxMeta)) break;
      const WalleRadioRxMeta* meta =
          reinterpret_cast<const WalleRadioRxMeta*>(payload);
      const size_t available = header->payloadLength - sizeof(WalleRadioRxMeta);
      if (meta->radioLength > available ||
          meta->radioLength > WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES) break;

      const uint8_t* radioData = payload + sizeof(WalleRadioRxMeta);
      s_lastRxMs = millis();
      ++s_rxCount;
      if (s_receiveCallback) {
        s_receiveCallback(meta->sourceMac, radioData, meta->radioLength,
                          meta->rssi, meta->channel);
      }
      break;
    }

    case WALLE_RADIO_MSG_STATUS: {
      if (header->payloadLength < sizeof(WalleRadioGatewayStatus)) break;
      const WalleRadioGatewayStatus* status =
          reinterpret_cast<const WalleRadioGatewayStatus*>(payload);
      s_gatewayReady = status->ready != 0;
      s_channel = status->channel;
      s_lastGatewayStatusMs = millis();
      break;
    }

    case WALLE_RADIO_MSG_HEARTBEAT:
      s_lastGatewayStatusMs = millis();
      break;

    default:
      break;
  }

  parserReset();
}

static void parserFeed(uint8_t value) {
  const uint8_t sofLo = (uint8_t)(WALLE_RADIO_BRIDGE_SOF & 0xFFu);
  const uint8_t sofHi = (uint8_t)(WALLE_RADIO_BRIDGE_SOF >> 8);

  switch (s_parserState) {
    case PARSER_WAIT_SOF_LO:
      if (value == sofLo) {
        s_frameBuffer[0] = value;
        s_framePos = 1;
        s_parserState = PARSER_WAIT_SOF_HI;
      }
      break;

    case PARSER_WAIT_SOF_HI:
      if (value == sofHi) {
        s_frameBuffer[s_framePos++] = value;
        s_parserState = PARSER_READ_HEADER;
      } else if (value == sofLo) {
        s_frameBuffer[0] = value;
        s_framePos = 1;
      } else {
        parserReset();
      }
      break;

    case PARSER_READ_HEADER:
      s_frameBuffer[s_framePos++] = value;
      if (s_framePos == sizeof(WalleRadioBridgeHeader)) {
        const WalleRadioBridgeHeader* header =
            reinterpret_cast<const WalleRadioBridgeHeader*>(s_frameBuffer);
        if (header->version != WALLE_RADIO_BRIDGE_VERSION ||
            header->payloadLength > WALLE_RADIO_BRIDGE_MAX_PAYLOAD) {
          parserReset();
          break;
        }
        s_frameTarget = sizeof(WalleRadioBridgeHeader) + header->payloadLength + 2;
        s_parserState = PARSER_READ_REST;
      }
      break;

    case PARSER_READ_REST:
      if (s_framePos >= sizeof(s_frameBuffer)) {
        parserReset();
        break;
      }
      s_frameBuffer[s_framePos++] = value;
      if (s_framePos == s_frameTarget) handleCompleteFrame();
      break;
  }
}

bool radioTransportInit(void) {
  if (s_started) return true;
  s_radioUart.begin(WALLE_RADIO_UART_BAUD, SERIAL_8N1,
                    WALLE_RADIO_UART_RX, WALLE_RADIO_UART_TX);
  s_started = true;
  s_gatewayReady = false;
  s_lastGatewayStatusMs = 0;
  s_lastControlTxMs = 0;
  parserReset();

  Serial.printf("[Radio/P4] UART1 gateway RX=%d TX=%d baud=%lu channel=%u\n",
                WALLE_RADIO_UART_RX, WALLE_RADIO_UART_TX,
                (unsigned long)WALLE_RADIO_UART_BAUD,
                (unsigned)WALLE_RADIO_CHANNEL);
  sendChannelRequest();
  return true;
}

void radioTransportPoll(void) {
  if (!s_started) return;
  while (s_radioUart.available() > 0) {
    const int value = s_radioUart.read();
    if (value >= 0) parserFeed((uint8_t)value);
  }

  const uint32_t now = millis();
  if (now - s_lastControlTxMs >= 1000u) {
    s_lastControlTxMs = now;
    if (!s_gatewayReady || now - s_lastGatewayStatusMs > 3000u) {
      sendChannelRequest();
    } else {
      (void)sendBridgeFrame(WALLE_RADIO_MSG_HEARTBEAT, nullptr, 0);
    }
  }

  if (s_gatewayReady && now - s_lastGatewayStatusMs > 3500u) {
    s_gatewayReady = false;
    Serial.println(F("[Radio/P4] Gateway heartbeat stale"));
  }
}

bool radioTransportSend(const uint8_t destinationMac[6],
                        const void* data,
                        size_t length) {
  if (!s_started || !destinationMac || !data || length == 0 ||
      length > WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES) {
    ++s_txFailures;
    return false;
  }

  uint8_t payload[sizeof(WalleRadioTxMeta) + WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES];
  WalleRadioTxMeta meta = {};
  memcpy(meta.destinationMac, destinationMac, 6);
  meta.channel = 0;
  meta.flags = isBroadcastMac(destinationMac) ? WALLE_RADIO_FLAG_BROADCAST : 0;
  meta.radioLength = (uint16_t)length;
  memcpy(payload, &meta, sizeof(meta));
  memcpy(payload + sizeof(meta), data, length);

  if (!sendBridgeFrame(WALLE_RADIO_MSG_TX_PACKET, payload,
                       (uint16_t)(sizeof(meta) + length))) {
    ++s_txFailures;
    return false;
  }
  ++s_txCount;
  return true;
}

bool radioTransportBroadcast(const void* data, size_t length) {
  static const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  return radioTransportSend(broadcastMac, data, length);
}

bool radioTransportIsReady(void) {
  return s_started && s_gatewayReady &&
         (millis() - s_lastGatewayStatusMs < 3500u);
}

#else

// ============================================================
// Native Wi-Fi ESP: ESP-NOW transport (S3 regression path)
// ============================================================

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static bool s_started = false;

static wifi_interface_t activeInterface(void) {
  const wifi_mode_t mode = WiFi.getMode();
  return (mode == WIFI_AP || mode == WIFI_AP_STA) ? WIFI_IF_AP : WIFI_IF_STA;
}

static bool ensurePeer(const uint8_t mac[6]) {
  if (!mac) return false;
  if (esp_now_is_peer_exist(mac)) return true;

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = activeInterface();
  const esp_err_t result = esp_now_add_peer(&peer);
  return result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST;
}

static void dispatchNativePacket(const uint8_t sourceMac[6],
                                 const uint8_t* data,
                                 int length,
                                 int8_t rssi) {
  if (!sourceMac || !data || length <= 0) return;
  s_lastRxMs = millis();
  ++s_rxCount;
  if (s_receiveCallback) {
    s_receiveCallback(sourceMac, data, (size_t)length, rssi, s_channel);
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void nativeReceive(const esp_now_recv_info_t* info,
                          const uint8_t* data,
                          int length) {
  if (!info) return;
  const int8_t rssi = info->rx_ctrl ? (int8_t)info->rx_ctrl->rssi : -127;
  dispatchNativePacket(info->src_addr, data, length, rssi);
}
#else
static void nativeReceive(const uint8_t* sourceMac,
                          const uint8_t* data,
                          int length) {
  // Arduino-ESP32 2.x receive callback does not expose RSSI metadata.
  dispatchNativePacket(sourceMac, data, length, -127);
}
#endif

bool radioTransportInit(void) {
  if (s_started) return true;
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[Radio] ESP-NOW init failed"));
    return false;
  }
  esp_now_register_recv_cb(nativeReceive);

  uint8_t primary = 0;
  wifi_second_chan_t secondary = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&primary, &secondary) == ESP_OK && primary != 0) {
    s_channel = primary;
  }

  s_started = true;
  Serial.printf("[Radio] Native ESP-NOW ready on channel %u\n",
                (unsigned)s_channel);
  return true;
}

void radioTransportPoll(void) {
  // Native ESP-NOW receive is callback-driven.
}

bool radioTransportSend(const uint8_t destinationMac[6],
                        const void* data,
                        size_t length) {
  if (!s_started || !destinationMac || !data || length == 0 || length > 250u) {
    ++s_txFailures;
    return false;
  }
  if (!ensurePeer(destinationMac)) {
    ++s_txFailures;
    return false;
  }

  const esp_err_t result = esp_now_send(
      destinationMac,
      reinterpret_cast<const uint8_t*>(data),
      length);
  if (result != ESP_OK) {
    ++s_txFailures;
    return false;
  }
  ++s_txCount;
  return true;
}

bool radioTransportBroadcast(const void* data, size_t length) {
  static const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  return radioTransportSend(broadcastMac, data, length);
}

bool radioTransportIsReady(void) {
  return s_started;
}

#endif
