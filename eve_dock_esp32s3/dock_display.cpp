#include "dock_display.h"
#include "pin_config.h"
#if DOCK_ENABLE_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <string.h>
#ifndef DOCK_OLED_VISIBLE_X
#define DOCK_OLED_VISIBLE_X 28
#endif
#ifndef DOCK_OLED_VISIBLE_Y
#define DOCK_OLED_VISIBLE_Y 24
#endif
static Adafruit_SSD1306 s_display(DOCK_OLED_WIDTH, DOCK_OLED_HEIGHT, &Wire, DOCK_OLED_RESET_PIN);
static bool s_displayOk = false;
static uint32_t s_lastDrawMs = 0;
static const char* short_state_name_(DockFsmState state) { switch (state) { case ST_IDLE: return "FAULT"; case ST_WAIT_FOR_DOCK: return "SCAN"; case ST_DOCKED: return "DOCKED"; case ST_CHARGING: return "CHARGING"; default: return "?"; } }
static void oledCursor(int16_t x, int16_t y) { s_display.setCursor(DOCK_OLED_VISIBLE_X + x, DOCK_OLED_VISIBLE_Y + y); }
static const char* fault_text_(uint32_t now_ms, DockFsmState state, bool chargingEnabled, bool chargeVoltageOk, uint32_t lastEveRxMs, bool chargeSafetyLocked, const char* chargeSafetyReason) {
  if (chargeSafetyLocked) {
    if (chargeSafetyReason && strstr(chargeSafetyReason, "pack reached")) return "E10 OV";
    if (chargeSafetyReason && strstr(chargeSafetyReason, "2 hour")) return "E11 2HR";
    return "E12 LOCK";
  }
  if (!chargeVoltageOk) return "E01 VBAT";
  if (lastEveRxMs == 0 && chargingEnabled) return "E02 INIT";
  if (lastEveRxMs == 0) return "E03 UART";
  if (state == ST_IDLE) return "E04 LOST";
  if ((state == ST_DOCKED || state == ST_CHARGING) && (uint32_t)(now_ms - lastEveRxMs) > DOCK_LOST_NO_RX_MS) return "E05 RX";
  if (state == ST_WAIT_FOR_DOCK) return "C01 EVE";
  if (state == ST_DOCKED && !chargingEnabled) return "W01 CHG";
  return "E00 OK";
}
void dockDisplayInit(void) { Wire.begin(DOCK_OLED_SDA, DOCK_OLED_SCL); Wire.setClock(400000); Wire.setTimeOut(50); s_displayOk = s_display.begin(SSD1306_SWITCHCAPVCC, DOCK_OLED_ADDR); if (!s_displayOk) { Serial.println(F("[OLED] SSD1306 init failed - continuing without display")); Serial.flush(); return; } s_display.clearDisplay(); s_display.setTextColor(SSD1306_WHITE); s_display.setTextSize(1); oledCursor(0, 0); s_display.println(F("EVE DOCK")); oledCursor(0, 10); s_display.println(F("BOOT")); s_display.display(); Serial.println(F("[OLED] init OK 128x64 windowed")); Serial.flush(); }
void dockDisplayUpdateEx(uint32_t now_ms, DockFsmState state, bool chargingEnabled, bool chargeVoltageOk, uint32_t lastEveRxMs, bool chargeSafetyLocked, const char* chargeSafetyReason) { if (!s_displayOk || (uint32_t)(now_ms - s_lastDrawMs) < DOCK_OLED_UPDATE_MS) return; s_lastDrawMs = now_ms; s_display.clearDisplay(); s_display.setTextColor(SSD1306_WHITE); s_display.setTextSize(1); oledCursor(0, 0); s_display.print(F("EVE DOCK")); oledCursor(0, 10); if (chargingEnabled) { s_display.print(F("CHARGING")); } else { s_display.print(F("MODE ")); s_display.print(short_state_name_(state)); } oledCursor(0, 20); s_display.print(fault_text_(now_ms, state, chargingEnabled, chargeVoltageOk, lastEveRxMs, chargeSafetyLocked, chargeSafetyReason)); oledCursor(0, 30); s_display.print(F("RX ")); if (lastEveRxMs == 0) s_display.print(F("WAIT")); else { s_display.print((now_ms - lastEveRxMs) / 1000u); s_display.print(F("s")); } s_display.display(); }
void dockDisplayUpdate(uint32_t now_ms, DockFsmState state, bool chargingEnabled, uint32_t lastEveRxMs) { dockDisplayUpdateEx(now_ms, state, chargingEnabled, true, lastEveRxMs); }
#else
void dockDisplayInit(void) {}
void dockDisplayUpdate(uint32_t now_ms, DockFsmState state, bool chargingEnabled, uint32_t lastEveRxMs) { (void)now_ms; (void)state; (void)chargingEnabled; (void)lastEveRxMs; }
void dockDisplayUpdateEx(uint32_t now_ms, DockFsmState state, bool chargingEnabled, bool chargeVoltageOk, uint32_t lastEveRxMs, bool chargeSafetyLocked, const char* chargeSafetyReason) { (void)now_ms; (void)state; (void)chargingEnabled; (void)chargeVoltageOk; (void)lastEveRxMs; (void)chargeSafetyLocked; (void)chargeSafetyReason; }
#endif
