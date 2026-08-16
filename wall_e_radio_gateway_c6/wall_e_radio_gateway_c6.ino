// ============================================================
// WALL-E ESP32-C6 Radio Gateway for ESP32-P4 Base Brain
//
// Purpose:
//   - Own ESP-NOW on behalf of the ESP32-P4 Base.
//   - Forward every received ESP-NOW packet to the P4 over UART.
//   - Accept opaque TX packets from the P4 and transmit them by ESP-NOW.
//   - Never interpret WALL-E commands; it is only a transport bridge.
//
// Wiring (defaults, change pins below to suit your gateway board):
//   P4 TX GPIO25  -> C6 RX GPIO17
//   P4 RX GPIO24  <- C6 TX GPIO16
//   P4 GND        -- C6 GND
//
// Target: ESP32-C6 Dev Module, Arduino-ESP32 3.x
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "../protocols/walle_radio_bridge_protocol.h"

// ------------------------------------------------------------
// User-adjustable gateway configuration
// ------------------------------------------------------------
#define GATEWAY_ESPNOW_CHANNEL  11
#define GATEWAY_UART_BAUD       921600UL
#define GATEWAY_UART_RX_PIN     17
#define GATEWAY_UART_TX_PIN     16
#define GATEWAY_STATUS_MS       1000UL
#define GATEWAY_RX_QUEUE_DEPTH  8

HardwareSerial BridgeSerial(1);

struct GatewayRxItem {
  uint8_t sourceMac[6];
  int8_t rssi;
  uint8_t channel;
  uint16_t length;
  uint8_t data[WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES];
};

static QueueHandle_t s_rxQueue = nullptr;
static bool s_radioReady = false;
static uint8_t s_channel = GATEWAY_ESPNOW_CHANNEL;
static uint16_t s_bridgeSequence = 0;
static uint32_t s_rxPackets = 0;
static uint32_t s_rxDrops = 0;
static uint32_t s_txPackets = 0;
static uint32_t s_txFailures = 0;
static uint32_t s_lastStatusMs = 0;

static uint8_t s_parserBuffer[sizeof(WalleRadioBridgeHeader) +
                              WALLE_RADIO_BRIDGE_MAX_PAYLOAD + 2];
static size_t s_parserPos = 0;
static size_t s_parserTarget = 0;

enum ParserState : uint8_t {
  PARSER_WAIT_SOF_LO = 0,
  PARSER_WAIT_SOF_HI,
  PARSER_READ_HEADER,
  PARSER_READ_REST
};
static ParserState s_parserState = PARSER_WAIT_SOF_LO;

// ============================================================
// Framed UART helpers
// ============================================================

static void parserReset() {
  s_parserState = PARSER_WAIT_SOF_LO;
  s_parserPos = 0;
  s_parserTarget = 0;
}

static bool bridgeWriteFrame(uint8_t type,
                             const uint8_t* payload,
                             uint16_t payloadLength) {
  if (payloadLength > WALLE_RADIO_BRIDGE_MAX_PAYLOAD) return false;

  WalleRadioBridgeHeader header = {};
  header.sof = WALLE_RADIO_BRIDGE_SOF;
  header.version = WALLE_RADIO_BRIDGE_VERSION;
  header.type = type;
  header.sequence = ++s_bridgeSequence;
  header.payloadLength = payloadLength;

  uint16_t crc = walleRadioCrc16(
      reinterpret_cast<const uint8_t*>(&header), sizeof(header));
  if (payload && payloadLength) {
    crc = walleRadioCrc16(payload, payloadLength, crc);
  }

  size_t written = BridgeSerial.write(
      reinterpret_cast<const uint8_t*>(&header), sizeof(header));
  if (payload && payloadLength) {
    written += BridgeSerial.write(payload, payloadLength);
  }

  const uint8_t crcBytes[2] = {
      (uint8_t)(crc & 0xFFu),
      (uint8_t)(crc >> 8)
  };
  written += BridgeSerial.write(crcBytes, sizeof(crcBytes));

  return written == sizeof(header) + payloadLength + sizeof(crcBytes);
}

static void bridgeSendStatus() {
  WalleRadioGatewayStatus status = {};
  status.ready = s_radioReady ? 1u : 0u;
  status.channel = s_channel;
  status.peerCount = 0; // ESP-NOW does not expose a cheap portable peer-count API.
  status.uptimeMs = millis();
  status.rxPackets = s_rxPackets;
  status.txPackets = s_txPackets;
  status.txFailures = s_txFailures + s_rxDrops;

  (void)bridgeWriteFrame(WALLE_RADIO_MSG_STATUS,
                         reinterpret_cast<const uint8_t*>(&status),
                         sizeof(status));
}

// ============================================================
// ESP-NOW helpers
// ============================================================

static bool setRadioChannel(uint8_t channel) {
  if (channel < 1 || channel > 13) return false;

  if (esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    return false;
  }
  s_channel = channel;
  Serial.printf("[Gateway] ESP-NOW channel -> %u\n", (unsigned)s_channel);
  return true;
}

static bool ensurePeer(const uint8_t mac[6]) {
  if (!mac) return false;
  if (esp_now_is_peer_exist(mac)) return true;

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = s_channel;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;

  const esp_err_t result = esp_now_add_peer(&peer);
  return result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST;
}

static void onEspNowReceive(const esp_now_recv_info_t* info,
                            const uint8_t* data,
                            int length) {
  if (!info || !data || length <= 0 ||
      length > (int)WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES || !s_rxQueue) {
    return;
  }

  GatewayRxItem item = {};
  memcpy(item.sourceMac, info->src_addr, 6);
  item.rssi = info->rx_ctrl ? (int8_t)info->rx_ctrl->rssi : -127;
  item.channel = s_channel;
  item.length = (uint16_t)length;
  memcpy(item.data, data, (size_t)length);

  if (xQueueSend(s_rxQueue, &item, 0) != pdTRUE) {
    ++s_rxDrops;
  }
}

static void onEspNowSent(const uint8_t* mac,
                         esp_now_send_status_t status) {
  (void)mac;
  if (status != ESP_NOW_SEND_SUCCESS) ++s_txFailures;
}

static bool initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);

  if (!setRadioChannel(GATEWAY_ESPNOW_CHANNEL)) {
    Serial.println(F("[Gateway] Failed to set Wi-Fi channel"));
    return false;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[Gateway] esp_now_init failed"));
    return false;
  }

  esp_now_register_recv_cb(onEspNowReceive);
  esp_now_register_send_cb(onEspNowSent);

  static const uint8_t broadcastMac[6] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };
  if (!ensurePeer(broadcastMac)) {
    Serial.println(F("[Gateway] Failed to add broadcast peer"));
    return false;
  }

  Serial.print(F("[Gateway] C6 MAC: "));
  Serial.println(WiFi.macAddress());
  Serial.printf("[Gateway] ESP-NOW ready on channel %u\n",
                (unsigned)s_channel);
  return true;
}

// ============================================================
// P4 -> gateway frame handling
// ============================================================

static void handleTxPacket(const uint8_t* payload, uint16_t payloadLength) {
  if (!payload || payloadLength < sizeof(WalleRadioTxMeta)) return;

  const WalleRadioTxMeta* meta =
      reinterpret_cast<const WalleRadioTxMeta*>(payload);
  const size_t available = payloadLength - sizeof(WalleRadioTxMeta);

  if (meta->radioLength == 0 ||
      meta->radioLength > available ||
      meta->radioLength > WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES) {
    ++s_txFailures;
    return;
  }

  if (meta->channel != 0 && meta->channel != s_channel) {
    if (!setRadioChannel(meta->channel)) {
      ++s_txFailures;
      return;
    }
  }

  if (!ensurePeer(meta->destinationMac)) {
    ++s_txFailures;
    return;
  }

  const uint8_t* radioData = payload + sizeof(WalleRadioTxMeta);
  const esp_err_t result = esp_now_send(meta->destinationMac,
                                        radioData,
                                        meta->radioLength);
  if (result == ESP_OK) ++s_txPackets;
  else ++s_txFailures;
}

static void handleBridgeFrame() {
  if (s_parserTarget < sizeof(WalleRadioBridgeHeader) + 2) {
    parserReset();
    return;
  }

  const WalleRadioBridgeHeader* header =
      reinterpret_cast<const WalleRadioBridgeHeader*>(s_parserBuffer);
  if (header->sof != WALLE_RADIO_BRIDGE_SOF ||
      header->version != WALLE_RADIO_BRIDGE_VERSION ||
      header->payloadLength > WALLE_RADIO_BRIDGE_MAX_PAYLOAD) {
    parserReset();
    return;
  }

  const size_t crcOffset = sizeof(WalleRadioBridgeHeader) +
                           header->payloadLength;
  const uint16_t expected = walleRadioCrc16(s_parserBuffer, crcOffset);
  const uint16_t received =
      (uint16_t)s_parserBuffer[crcOffset] |
      ((uint16_t)s_parserBuffer[crcOffset + 1] << 8);

  if (expected != received) {
    Serial.println(F("[Gateway] UART frame CRC error"));
    parserReset();
    return;
  }

  const uint8_t* payload =
      s_parserBuffer + sizeof(WalleRadioBridgeHeader);

  switch (header->type) {
    case WALLE_RADIO_MSG_TX_PACKET:
      handleTxPacket(payload, header->payloadLength);
      break;

    case WALLE_RADIO_MSG_SET_CHANNEL:
      if (header->payloadLength >= 1) {
        (void)setRadioChannel(payload[0]);
        bridgeSendStatus();
      }
      break;

    case WALLE_RADIO_MSG_HEARTBEAT:
      bridgeSendStatus();
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
        s_parserBuffer[0] = value;
        s_parserPos = 1;
        s_parserState = PARSER_WAIT_SOF_HI;
      }
      break;

    case PARSER_WAIT_SOF_HI:
      if (value == sofHi) {
        s_parserBuffer[s_parserPos++] = value;
        s_parserState = PARSER_READ_HEADER;
      } else if (value == sofLo) {
        s_parserBuffer[0] = value;
        s_parserPos = 1;
      } else {
        parserReset();
      }
      break;

    case PARSER_READ_HEADER:
      s_parserBuffer[s_parserPos++] = value;
      if (s_parserPos == sizeof(WalleRadioBridgeHeader)) {
        const WalleRadioBridgeHeader* header =
            reinterpret_cast<const WalleRadioBridgeHeader*>(s_parserBuffer);
        if (header->version != WALLE_RADIO_BRIDGE_VERSION ||
            header->payloadLength > WALLE_RADIO_BRIDGE_MAX_PAYLOAD) {
          parserReset();
          return;
        }
        s_parserTarget = sizeof(WalleRadioBridgeHeader) +
                         header->payloadLength + 2;
        s_parserState = PARSER_READ_REST;
      }
      break;

    case PARSER_READ_REST:
      if (s_parserPos >= sizeof(s_parserBuffer)) {
        parserReset();
        return;
      }
      s_parserBuffer[s_parserPos++] = value;
      if (s_parserPos == s_parserTarget) handleBridgeFrame();
      break;
  }
}

// ============================================================
// Gateway -> P4 RX forwarding
// ============================================================

static void forwardQueuedRadioPackets() {
  if (!s_rxQueue) return;

  GatewayRxItem item = {};
  while (xQueueReceive(s_rxQueue, &item, 0) == pdTRUE) {
    uint8_t payload[sizeof(WalleRadioRxMeta) +
                    WALLE_RADIO_BRIDGE_MAX_RADIO_BYTES];

    WalleRadioRxMeta meta = {};
    memcpy(meta.sourceMac, item.sourceMac, 6);
    meta.rssi = item.rssi;
    meta.channel = item.channel;
    meta.radioLength = item.length;

    memcpy(payload, &meta, sizeof(meta));
    memcpy(payload + sizeof(meta), item.data, item.length);

    if (bridgeWriteFrame(WALLE_RADIO_MSG_RX_PACKET,
                         payload,
                         (uint16_t)(sizeof(meta) + item.length))) {
      ++s_rxPackets;
    } else {
      ++s_rxDrops;
    }
  }
}

// ============================================================
// Arduino setup / loop
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n[Gateway] WALL-E ESP32-C6 radio gateway"));

  BridgeSerial.begin(GATEWAY_UART_BAUD,
                     SERIAL_8N1,
                     GATEWAY_UART_RX_PIN,
                     GATEWAY_UART_TX_PIN);
  parserReset();

  s_rxQueue = xQueueCreate(GATEWAY_RX_QUEUE_DEPTH,
                           sizeof(GatewayRxItem));
  if (!s_rxQueue) {
    Serial.println(F("[Gateway] FATAL: RX queue allocation failed"));
    while (true) delay(1000);
  }

  s_radioReady = initEspNow();
  bridgeSendStatus();
}

void loop() {
  while (BridgeSerial.available() > 0) {
    const int value = BridgeSerial.read();
    if (value >= 0) parserFeed((uint8_t)value);
  }

  forwardQueuedRadioPackets();

  const uint32_t now = millis();
  if (now - s_lastStatusMs >= GATEWAY_STATUS_MS) {
    s_lastStatusMs = now;
    bridgeSendStatus();

    Serial.printf("[Gateway] ready=%u ch=%u rx=%lu drop=%lu tx=%lu fail=%lu heap=%lu\n",
                  s_radioReady ? 1u : 0u,
                  (unsigned)s_channel,
                  (unsigned long)s_rxPackets,
                  (unsigned long)s_rxDrops,
                  (unsigned long)s_txPackets,
                  (unsigned long)s_txFailures,
                  (unsigned long)ESP.getFreeHeap());
  }

  delay(1);
}
