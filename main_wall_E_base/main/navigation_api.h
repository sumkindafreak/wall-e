#pragma once

#include <Arduino.h>

/** POST /api/navigation/route — body: JSON (see navigation_api.cpp). */
void navigationHandleRoutePost();

/** GET /api/navigation/status */
void navigationHandleStatusGet();

/** GET /api/navigation/stop — stop following waypoints (does not disable autonomy). */
void navigationHandleStopGet();
