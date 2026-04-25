#include "dock_display.h"
#include "pin_config.h"
#if DOCK_ENABLE_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
static Adafruit_SSD1306 s_display(DOCK_OLED_WIDTH, DOCK_OLED_HEIGHT, &Wire, DOCK_OLED_RESET_PIN);
static bool s_displayOk = false;
static uint32_t s_lastDrawMs = 0;
static const char* short_state_name_(DockFsmState state) { switch (state) { case ST_IDLE: return "FAULT"; case ST_WAIT_FOR_DOCK: return "SCAN"; case ST_DOCKED: return "DOCKED"; case ST_CHARGING: return "CHARGING"; default: return "?"; } }
void dockDisplayInit(void) { Wire.begin(DOCK_OLED_SDA, DOCK_OLED_SCL); Wire.setClock(400000); Wire.setTimeOut(50); s_displayOk = s_display.begin(SSD1306_SWITCHCAPVCC, DOCK_OLED_ADDR); if (!s_displayOk) { Serial.println(F("[OLED] SSD1306 init failed - continuing without display")); Serial.flush(); return; } s_display.clearDisplay(); s_display.setTextColor(SSD1306_WHITE); s_display.setTextSize(1); s_display.setCursor(0, 0); s_display.println(F("EVE DOCK")); s_display.println(F("Booting...")); s_display.display(); Serial.println(F("[OLED] init OK")); Serial.flush(); }
void dockDisplayUpdate(uint32_t now_ms, DockFsmState state, bool chargingEnabled, uint32_t lastEveRxMs) { if (!s_displayOk || (uint32_t)(now_ms - s_lastDrawMs) < DOCK_OLED_UPDATE_MS) return; s_lastDrawMs = now_ms; s_display.clearDisplay(); s_display.setTextColor(SSD1306_WHITE); s_display.setTextSize(1); s_display.setCursor(0, 0); s_display.print(F("EVE DOCK")); s_display.setCursor(0, 10); if (chargingEnabled) { s_display.print(F("CHARGING")); s_display.setCursor(0, 20); s_display.print(F("MOSFET ON")); } else { s_display.print(F("MODE ")); s_display.print(short_state_name_(state)); s_display.setCursor(0, 20); s_display.print(F("CHG OFF")); } s_display.setCursor(0, 30); s_display.print(F("RX ")); if (lastEveRxMs == 0) s_display.print(F("WAIT")); else { s_display.print((now_ms - lastEveRxMs) / 1000u); s_display.print(F("s")); } s_display.display(); }
#else
void dockDisplayInit(void) {}
void dockDisplayUpdate(uint32_t now_ms, DockFsmState state, bool chargingEnabled, uint32_t lastEveRxMs) { (void)now_ms; (void)state; (void)chargingEnabled; (void)lastEveRxMs; }
#endif
