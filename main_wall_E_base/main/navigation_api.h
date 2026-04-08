#pragma once

#include <Arduino.h>

/** POST /api/navigation/route — body: JSON (see navigation_api.cpp). */
void navigationHandleRoutePost();

/** GET /api/navigation/status */
void navigationHandleStatusGet();

/** GET /api/navigation/stop — stop following waypoints (does not disable autonomy). */
void navigationHandleStopGet();

/**
 * Apply the same JSON body as POST /api/navigation/route (grid or gps frame).
 * Used by the sequence engine and HTTP handler. errBuf optional (short reason).
 */
bool navigationApplyRouteFromJson(const char* body, size_t bodyLen, char* errBuf, size_t errBufLen);
