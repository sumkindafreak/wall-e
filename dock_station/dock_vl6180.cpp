/*******************************************************************************
 * VL6180X (TOF050C) — dock slot presence via I2C.
 ******************************************************************************/

#include "dock_vl6180.h"
#include "dock_config.h"
#include <Arduino.h>

#if USE_VL6180_TOF

#include <Wire.h>
#include "Adafruit_VL6180X.h"

static Adafruit_VL6180X s_vl;
static uint8_t s_mm = 0;
static bool s_ok = false; /* hardware present and begin() OK */
static uint32_t s_last_ms = 0;

#define VL6180_READ_INTERVAL_MS  80

void dockVl6180Begin(void) {
  Wire.begin(PIN_VL6180_SDA, PIN_VL6180_SCL);
  Wire.setClock(100000);
  delay(50);
  if (!s_vl.begin()) {
    Serial.println(F("[VL6180] init FAILED — check SDA/SCL and 3.3V"));
    s_ok = false;
    return;
  }
  s_ok = true;
  Serial.printf("[VL6180] OK SDA=%d SCL=%d (dock if %d–%d mm)\n",
                PIN_VL6180_SDA, PIN_VL6180_SCL,
                (int)VL6180_DOCK_MIN_MM, (int)VL6180_DOCK_MAX_MM);
}

void dockVl6180Update(void) {
  if (!s_ok) {
    return;
  }
  uint32_t now = millis();
  if (now - s_last_ms < VL6180_READ_INTERVAL_MS) {
    return;
  }
  s_last_ms = now;

  uint8_t mm = s_vl.readRange();
  (void)s_vl.readRangeStatus();
  /* 255 = no target / error on VL6180X */
  if (mm == 255 || mm < 5) {
    s_mm = 0;
    return;
  }
  s_mm = mm;
}

uint8_t dockVl6180RangeMm(void) {
  return s_mm;
}

bool dockVl6180Docked(void) {
  if (!s_ok || s_mm == 0) {
    return false;
  }
  return (s_mm >= VL6180_DOCK_MIN_MM) && (s_mm <= VL6180_DOCK_MAX_MM);
}

bool dockVl6180Ready(void) {
  return s_ok;
}

#else /* !USE_VL6180_TOF */

void dockVl6180Begin(void) {}
void dockVl6180Update(void) {}
uint8_t dockVl6180RangeMm(void) { return 0; }
bool dockVl6180Docked(void) { return false; }
bool dockVl6180Ready(void) { return false; }

#endif
