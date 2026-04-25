#pragma once

#include <Arduino.h>

void eveAttachmentManagerInit(void);
void eveAttachmentManagerTick(void);

/** True if present pin wired and grounded when mated to WALL-E. */
bool eveAttachmentIsAttached(void);
