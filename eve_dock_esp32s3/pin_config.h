#pragma once
#include <Arduino.h>

// Set to 1 for first board bring-up. It skips OLED/NeoPixel/MOSFET/UART init
// and only prints serial heartbeat messages from setup()/loop().
#define DOCK_SAFE_BOOT_ONLY 1

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
#define DOCK_OLED_VISIBLE_Y 12
#define DOCK_OLED_RESET_PIN (-1)
#define DOCK_OLED_UPDATE_MS 250u

#define DOCK_PIN_CHG_GATE 10
#define DOCK_CHG_ACTIVE_HIGH 1
#define DOCK_PIN_BUTTON (-1)
#define DOCK_BUTTON_ACTIVE_LOW 1
#define DOCK_PIN_NEOPIXEL 7
#define DOCK_NEO_COUNT 1u
#define DOCK_PIN_VBAT_SENSE (-1)
#define DOCK_VBAT_SENSE_OK_MIN 1200u
#define DOCK_VBAT_SENSE_OK_MAX 3000u
#define DOCK_MAX_WAIT_MS 30000u
#define DOCK_LOST_NO_RX_MS 10000u
#define DOCK_UART_MAX_BYTES_PER_LOOP 64u
