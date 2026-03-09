#pragma once

// ============================================================
//  WALL-E Display Manager
//  ST7789 240x240 via TFT_eSPI (no CS pin)
// ============================================================

#include <Arduino.h>

// --- Pin Definitions (match working display_control.cpp pattern) ---
#define TFT_SDA_PIN   11    // MOSI
#define TFT_SCL_PIN   12    // SCLK
#define TFT_DC_PIN    13    // Data/Command
#define TFT_RES_PIN   14    // Reset
#define TFT_BLK_PIN   15    // Backlight (PWM)

// Aliases for Adafruit_ST7789 init (same as display_control.cpp)
#define TFT_MOSI      TFT_SDA_PIN
#define TFT_SCK       TFT_SCL_PIN
#define TFT_DC        TFT_DC_PIN
#define TFT_RST       TFT_RES_PIN
#define TFT_BL        TFT_BLK_PIN
#define TFT_CS        -1    // No CS pin — tied to GND on board
#define TFT_SPI_FREQ  40000000

// Backlight PWM
#define BLK_PWM_CHANNEL   2
#define BLK_PWM_FREQ      5000
#define BLK_PWM_RES       8
#define BLK_BRIGHTNESS    220   // 0-255

// Drive command states — mirrors web commands
typedef enum {
  CMD_IDLE    = 0,
  CMD_FORWARD,
  CMD_REVERSE,
  CMD_LEFT,
  CMD_RIGHT,
  CMD_STOP,
  CMD_DRIVE   // tank: stick at _joyX, _joyY
} DriveCommand;

void displayInit();
void displaySetCommand(DriveCommand cmd);
void displaySetSpeed(uint8_t speed);
void displaySetStick(float jx, float jy);  // -1..1 tank stick position for smooth joystick display
void displayUpdateWifi();
void displayUpdateBattery();
void displayHandle();
