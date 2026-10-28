/**
 * EVE side of the WALL-E hand UART.
 *
 * Framing, CRC and validation live in firmware_common/include/eve_link_protocol.h (shared with the
 * WALL-E base). Only CRC-valid frames with the supported protocol version reach the callback, so
 * line noise and malformed packets can never look like a live link.
 */
#include "uart_link.h"
#include "config.h"
#include <HardwareSerial.h>
#include <string.h>

static EveUartRxCallback s_rxCb = nullptr;
static uint8_t s_txSeq = 0;
static EveFrameDecoder s_decoder;

/** Upper bound of bytes handled per uartLinkPoll() call (~44 ms of 115200 baud traffic). */
static const int EVE_UART_MAX_BYTES_PER_POLL = 512;

void uartLinkInit(void) {
  /* Both buffer sizes must be set before begin() on ESP32 Arduino Core 3.x. The TX buffer lets a
   * ~450-byte heartbeat leave in the background instead of blocking the loop for ~40 ms. */
  Serial1.setRxBufferSize(1024);
  Serial1.setTxBufferSize(2048);
  Serial1.begin(EVE_UART_BAUD, SERIAL_8N1, EVE_UART_RX_PIN, EVE_UART_TX_PIN);
  s_decoder.reset();
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
  const EveFrameDecoder::Stats& s = s_decoder.stats();
  return s.crcErr + s.versionErr + s.lengthErr;
}

uint32_t uartLinkFramesOk(void) {
  return s_decoder.stats().ok;
}

const EveFrameDecoder::Stats& uartLinkStats(void) {
  return s_decoder.stats();
}

void uartLinkPoll(void) {
  int budget = EVE_UART_MAX_BYTES_PER_POLL;
  while (budget-- > 0 && Serial1.available() > 0) {
    const uint8_t c = (uint8_t)Serial1.read();
    if (s_decoder.feed(c) && s_rxCb) {
      s_rxCb(s_decoder.type(), s_decoder.payload(), s_decoder.len(), s_decoder.seq());
    }
  }
}

bool uartLinkSendJson(EveMsgType type, const char* jsonUtf8) {
  if (!jsonUtf8) {
    return false;
  }
  const size_t plen = strlen(jsonUtf8);
  if (plen > EVE_MAX_PAYLOAD) {
    return false;
  }
  static uint8_t frame[EVE_MAX_PAYLOAD + EVE_FRAME_OVERHEAD];
  const size_t n = eveLinkEncode((uint8_t)type, uartLinkNextSeq(), (const uint8_t*)jsonUtf8, (uint16_t)plen, frame,
                                 sizeof(frame));
  if (n == 0) {
    return false;
  }
  return Serial1.write(frame, n) == n;
}
