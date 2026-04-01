// ============================================================
//  Link + telemetry freshness for SYSTEM page / debug
// ============================================================

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void systemStatusInit(void);
void systemStatusTick(uint32_t now_ms);

bool systemStatusEspNowLinkOk(void);
bool systemStatusTelemetryFresh(uint32_t maxAgeMs);

/** One-line debug for optional overlay / Serial */
void systemStatusFormatDebug(char* buf, size_t bufLen);
