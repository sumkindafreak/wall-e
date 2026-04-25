#include "eve_attachment_manager.h"
#include "config.h"

void eveAttachmentManagerInit(void) {
#if EVE_PRESENT_PIN >= 0
  pinMode(EVE_PRESENT_PIN, INPUT_PULLUP);
#endif
  Serial.println(F("[BOOT][EVE] attachment manager"));
}

void eveAttachmentManagerTick(void) {}

bool eveAttachmentIsAttached(void) {
#if EVE_PRESENT_PIN >= 0
  /* Adjust if your harness is active-high instead of pull-up to GND when docked */
  return digitalRead(EVE_PRESENT_PIN) == LOW;
#else
  return false;
#endif
}
