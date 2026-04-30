#pragma once
#include <Arduino.h>

// Set to 1 for first board bring-up. It skips OLED/NeoPixel/MOSFET/UART init
// and only prints serial heartbeat messages from setup()/loop().
#define DOCK_SAFE_BOOT_ONLY 0

#define DOCK_UART_BAUD 115200u
// Avoid GPIO20/21 during bring-up; some ESP32-C3 board profiles use them for serial.
#define DOCK_PIN_UART_TX 4
#define DOCK_PIN_UART_RX 3
#define DOCK_HW_SERIAL Serial1

#define DOCK_ENABLE_OLED 1
#define DOCK_OLED_SDA 5
#define DOCK_OLED_SCL 6
#define DOCK_OLED_ADDR 0x3C
// The 0.42" 72x40 glass is usually a window inside a 128x64 SSD1306 controller.
// Use a 128x64 framebuffer and draw into the visible window offset.
#define DOCK_OLED_WIDTH 128
#define DOCK_OLED_HEIGHT 64
#define DOCK_OLED_VISIBLE_X 28
#define DOCK_OLED_VISIBLE_Y 24
#define DOCK_OLED_RESET_PIN (-1)
#define DOCK_OLED_UPDATE_MS 250u

#define DOCK_PIN_CHG_GATE 10
#define DOCK_CHG_ACTIVE_HIGH 1
// On boot, briefly enable charge so a fully flat EVE can wake enough to handshake.
#define DOCK_STARTUP_CHARGE_PRIME_MS 30000u
// If EVE is too flat to report UART/battery activity, keep charging on and red-flash while waiting.
#define DOCK_KEEP_CHARGE_ON_NO_EVE_ACTIVITY 1
#define DOCK_PIN_BUTTON (-1)
#define DOCK_BUTTON_ACTIVE_LOW 1
#define DOCK_PIN_NEOPIXEL 7
#define DOCK_NEO_COUNT 2u
// ESP32-C3 OLED Super Mini boards often have an onboard WS2812/RGB LED on GPIO8.
#define DOCK_ONBOARD_RGB_HEARTBEAT 1
#define DOCK_PIN_ONBOARD_RGB 8
#define DOCK_PIN_VBAT_SENSE (-1)
#define DOCK_VBAT_SENSE_OK_MIN 1200u
#define DOCK_VBAT_SENSE_OK_MAX 3000u
// Hard charge safety: MOSFET off at max pack V (match EVE 2S limits) or after 2 hours.
// Uses EVE UART battery voltage; optional local ADC can be calibrated separately.
#define DOCK_CHARGE_MAX_PACK_V 8.4f
#define DOCK_CHARGE_MAX_SESSION_MS 7200000u
#define DOCK_MAX_WAIT_MS 30000u
#define DOCK_LOST_NO_RX_MS 10000u
#define DOCK_UART_MAX_BYTES_PER_LOOP 64u
