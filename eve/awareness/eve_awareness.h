/**
 * EVE Phase O — Awareness layer public API.
 * Read-only facts about the world; no decisions or side effects.
 */
#pragma once

#include "eve_awareness_snapshot.h"

void eveAwarenessInit(void);
void eveAwarenessTick(void);

const EveAwarenessSnapshot& eveAwarenessGetSnapshot(void);

/** Bench: formatted serial dump (rate-limited inside tick when enabled). */
void eveAwarenessPrintSerial(void);
