#include "eve_attachment_manager.h"

#include "config.h"

#if EVE_PRESENT_PIN >= 0
static bool s_stable = false;
static bool s_lastRaw = false;
static uint32_t s_changeMs = 0;
static const uint32_t kDebounceMs = 60;

static bool readRaw(void) {
  const int level = digitalRead(EVE_PRESENT_PIN);
  return EVE_PRESENT_ACTIVE_LOW ? (level == LOW) : (level == HIGH);
}
#endif

void eveAttachmentManagerInit(void) {
#if EVE_PRESENT_PIN >= 0
  pinMode(EVE_PRESENT_PIN, EVE_PRESENT_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
  s_lastRaw = s_stable = readRaw();
  s_changeMs = millis();
  Serial.printf("[BOOT][EVE] presence pin GPIO%d (%s): %s\n", EVE_PRESENT_PIN,
                EVE_PRESENT_ACTIVE_LOW ? "active low" : "active high", s_stable ? "present" : "not present");
#else
  Serial.println(F("[BOOT][EVE] no presence pin - physical contact is reported as unknown"));
#endif
}

void eveAttachmentManagerTick(void) {
#if EVE_PRESENT_PIN >= 0
  const bool raw = readRaw();
  const uint32_t now = millis();
  if (raw != s_lastRaw) {
    s_lastRaw = raw;
    s_changeMs = now;
  } else if (raw != s_stable && (uint32_t)(now - s_changeMs) >= kDebounceMs) {
    s_stable = raw;
    Serial.printf("[EVE][PRESENCE] %s\n", s_stable ? "present (pin)" : "not present (pin)");
  }
#endif
}

bool eveAttachmentIsAttached(void) {
#if EVE_PRESENT_PIN >= 0
  return s_stable;
#else
  return false;
#endif
}

int8_t eveAttachmentPresence(void) {
#if EVE_PRESENT_PIN >= 0
  return s_stable ? 1 : 0;
#else
  return -1;
#endif
}
