#include "eve_feedback.h"

#include <string.h>

static uint32_t s_lastToastMs;
static char s_lastBuf[56];

void eveFeedbackInit(void) {
  s_lastToastMs = 0;
  memset(s_lastBuf, 0, sizeof(s_lastBuf));
}

void showToast(const char* message) {
  if (!message || !message[0]) {
    return;
  }
  const uint32_t now = millis();
  if ((uint32_t)(now - s_lastToastMs) < 140u) {
    return;
  }
  if (strncmp(message, s_lastBuf, sizeof(s_lastBuf) - 1) == 0 &&
      (uint32_t)(now - s_lastToastMs) < 2200u) {
    return;
  }

  strncpy(s_lastBuf, message, sizeof(s_lastBuf) - 1);
  s_lastBuf[sizeof(s_lastBuf) - 1] = '\0';
  s_lastToastMs = now;

  Serial.print(F("[EVE][toast] "));
  Serial.println(message);
}
