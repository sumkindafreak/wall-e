#pragma once

// ============================================================
// WALL-E Base display abstraction
//
// Legacy S3 Base: external ST7789 240x240.
// ESP32-P4 Base: headless by default; CYD/LROS remain operator displays.
// A future P4 DSI display can implement this same API without changing the
// robot logic.
// ============================================================

#include <Arduino.h>
#include "base_board_pins.h"

#if WALLE_BASE_LOCAL_TFT
#include "ledc_compat.h"

#define TFT_MOSI      BASE_PIN_TFT_MOSI
#define TFT_SCK       BASE_PIN_TFT_SCK
#define TFT_DC        BASE_PIN_TFT_DC
#define TFT_RST       BASE_PIN_TFT_RST
#define TFT_BL        BASE_PIN_TFT_BL
#define TFT_CS        -1
#define TFT_SPI_FREQ  40000000

#define BLK_PWM_CHANNEL   WALLE_LEDC_CH_DISPLAY_BL
#define BLK_PWM_FREQ      5000
#define BLK_PWM_RES       8
#define BLK_BRIGHTNESS    220

// display_manager.cpp predates Arduino-ESP32 3.x and historically calls the
// pin-based LEDC API directly. On Arduino-ESP32 2.x only, remap those two
// display-local calls through our explicit channel compatibility shim. Arduino
// headers have already been parsed above, so these macros do not alter core
// declarations.
#if ESP_ARDUINO_VERSION_MAJOR < 3
#define ledcAttach(pin, freq, resolution) \
  walleLedcAttach((pin), WALLE_LEDC_CH_DISPLAY_BL, (freq), (resolution))
#define ledcWrite(pin, duty) \
  walleLedcWrite((pin), WALLE_LEDC_CH_DISPLAY_BL, (duty))
#endif
#endif

typedef enum {
  CMD_IDLE = 0,
  CMD_FORWARD,
  CMD_REVERSE,
  CMD_LEFT,
  CMD_RIGHT,
  CMD_STOP,
  CMD_DRIVE
} DriveCommand;

void displayInit();
void displaySetCommand(DriveCommand cmd);
void displaySetSpeed(uint8_t speed);
void displaySetStick(float jx, float jy);
void displayUpdateWifi();
void displayUpdateBattery();
void displayHandle();
void displayShowToast(const char* msg);
