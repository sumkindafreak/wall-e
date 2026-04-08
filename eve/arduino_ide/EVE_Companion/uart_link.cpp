/**
 * UART framing: SOF, version, type, length (LE), seq, payload, CRC-16-CCITT over [VER..payload end].
 */
#include "uart_link.h"
#include "config.h"
#include <string.h>

static EveUartRxCallback s_rxCb = nullptr;
static uint8_t s_txSeq = 0;
static uint32_t s_rxErr = 0;
static uint32_t s_rxOk = 0;

enum ParseState : uint8_t {
  ST_SOF0 = 0,
  ST_SOF1,
  ST_VER,
  ST_TYPE,
  ST_LEN_LO,
  ST_LEN_HI,
  ST_SEQ,
  ST_PAYLOAD,
  ST_CRC_LO,
  ST_CRC_HI
};

static ParseState st = ST_SOF0;
static uint8_t fVer = 0;
static uint8_t fType = 0;
static uint16_t fLen = 0;
static uint8_t fSeq = 0;
static uint16_t fIdx = 0;
static uint8_t fPayload[EVE_MAX_PAYLOAD];
static uint8_t fCrcLo = 0;
static uint8_t fCrcHi = 0;

static uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int b = 0; b < 8; b++) {
      if (crc & 0x8000) {
        crc = (uint16_t)((crc << 1) ^ 0x1021);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void uartLinkInit(void) {
  Serial1.begin(EVE_UART_BAUD, SERIAL_8N1, EVE_UART_RX_PIN, EVE_UART_TX_PIN);
  Serial1.setRxBufferSize(1024);
  st = ST_SOF0;
  Serial.print(F("[EVE][UART] Serial1 RX="));
  Serial.print(EVE_UART_RX_PIN);
  Serial.print(F(" TX="));
  Serial.print(EVE_UART_TX_PIN);
  Serial.print(F(" baud="));
  Serial.println(EVE_UART_BAUD);
}

void uartLinkSetRxCallback(EveUartRxCallback cb) {
  s_rxCb = cb;
}

uint8_t uartLinkNextSeq(void) {
  s_txSeq++;
  return s_txSeq;
}

uint32_t uartLinkRxErrors(void) {
  return s_rxErr;
}

uint32_t uartLinkFramesOk(void) {
  return s_rxOk;
}

static void resetParser(void) {
  st = ST_SOF0;
}

static void deliverIfCrcOk(void) {
  uint8_t buf[5 + EVE_MAX_PAYLOAD];
  buf[0] = fVer;
  buf[1] = fType;
  buf[2] = (uint8_t)(fLen & 0xFF);
  buf[3] = (uint8_t)((fLen >> 8) & 0xFF);
  buf[4] = fSeq;
  if (fLen > 0) {
    memcpy(buf + 5, fPayload, fLen);
  }
  uint16_t calc = crc16_ccitt(buf, (size_t)(5 + fLen));
  uint16_t wire = (uint16_t)fCrcLo | ((uint16_t)fCrcHi << 8);
  if (calc != wire) {
    s_rxErr++;
    resetParser();
    return;
  }
  s_rxOk++;
  if (s_rxCb) {
    s_rxCb(fType, fPayload, fLen, fSeq);
  }
  resetParser();
}

void uartLinkPoll(void) {
  while (Serial1.available() > 0) {
    uint8_t c = (uint8_t)Serial1.read();
    switch (st) {
      case ST_SOF0:
        if (c == EVE_SOF0) {
          st = ST_SOF1;
        }
        break;
      case ST_SOF1:
        if (c == EVE_SOF1) {
          st = ST_VER;
        } else if (c == EVE_SOF0) {
          st = ST_SOF1;
        } else {
          st = ST_SOF0;
        }
        break;
      case ST_VER:
        fVer = c;
        st = ST_TYPE;
        break;
      case ST_TYPE:
        fType = c;
        st = ST_LEN_LO;
        break;
      case ST_LEN_LO:
        fLen = c;
        st = ST_LEN_HI;
        break;
      case ST_LEN_HI:
        fLen |= (uint16_t)c << 8;
        if (fLen > EVE_MAX_PAYLOAD) {
          s_rxErr++;
          resetParser();
        } else {
          st = ST_SEQ;
        }
        break;
      case ST_SEQ:
        fSeq = c;
        fIdx = 0;
        if (fLen == 0) {
          st = ST_CRC_LO;
        } else {
          st = ST_PAYLOAD;
        }
        break;
      case ST_PAYLOAD:
        fPayload[fIdx++] = c;
        if (fIdx >= fLen) {
          st = ST_CRC_LO;
        }
        break;
      case ST_CRC_LO:
        fCrcLo = c;
        st = ST_CRC_HI;
        break;
      case ST_CRC_HI:
        fCrcHi = c;
        deliverIfCrcOk();
        break;
      default:
        resetParser();
        break;
    }
  }
}

bool uartLinkSendJson(EveMsgType type, const char* jsonUtf8) {
  if (!jsonUtf8) {
    return false;
  }
  size_t plen = strlen(jsonUtf8);
  if (plen > EVE_MAX_PAYLOAD) {
    return false;
  }
  uint8_t seq = uartLinkNextSeq();
  uint8_t buf[5 + EVE_MAX_PAYLOAD + 2];
  buf[0] = EVE_FRAME_VER;
  buf[1] = (uint8_t)type;
  buf[2] = (uint8_t)(plen & 0xFF);
  buf[3] = (uint8_t)((plen >> 8) & 0xFF);
  buf[4] = seq;
  if (plen > 0) {
    memcpy(buf + 5, jsonUtf8, plen);
  }
  uint16_t crc = crc16_ccitt(buf, 5 + plen);
  buf[5 + plen] = (uint8_t)(crc & 0xFF);
  buf[5 + plen + 1] = (uint8_t)((crc >> 8) & 0xFF);
  Serial1.write(EVE_SOF0);
  Serial1.write(EVE_SOF1);
  Serial1.write(buf, 5 + plen + 2);
  return true;
}
