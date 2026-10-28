/**
 * Optional presence input (EVE_PRESENT_PIN). This is the ONLY thing in EVE's firmware that can
 * say she is physically mated to WALL-E's hand. A working UART link alone proves communication,
 * not contact. Without the pin, presence is reported as "unknown" (-1).
 */
#pragma once

#include <Arduino.h>

void eveAttachmentManagerInit(void);
/** Debounces the presence pin (call from loop). */
void eveAttachmentManagerTick(void);

/** True only if the presence pin is wired AND reads "present" (debounced). */
bool eveAttachmentIsAttached(void);
/** -1 = no presence pin configured, 0 = not present, 1 = present. */
int8_t eveAttachmentPresence(void);
