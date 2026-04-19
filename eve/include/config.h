/**
 * EVE companion — single place for pins and feature flags.
 * ESP32-S3 N16R8 full PCB: values MUST match your schematic (defaults = DevKitC-1 style UART1).
 */
#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// UART hand link to WALL-E (full duplex, 3.3 V, common GND on pogo)
// Crossover: WALL-E TX -> EVE RX, WALL-E RX <- EVE TX
// -----------------------------------------------------------------------------
static const unsigned long EVE_UART_BAUD = 115200;
// UART1: default GPIO17 = TX (from EVE toward WALL-E), GPIO18 = RX (from WALL-E into EVE)
#define EVE_UART_TX_PIN 17
#define EVE_UART_RX_PIN 18

// Optional 4th pogo: digital presence (EVE input). Set to -1 if not wired.
#define EVE_PRESENT_PIN (-1)

// -----------------------------------------------------------------------------
// DFPlayer Mini (second UART — MUST NOT share pins with hand UART)
// Set pins when wired; leave RX/TX at -1 until then.
// -----------------------------------------------------------------------------
#define EVE_DFPLAYER_UART_RX (-1)
#define EVE_DFPLAYER_UART_TX (-1)
#define EVE_DFPLAYER_BAUD 9600

// -----------------------------------------------------------------------------
// Shared SPI for dual eye TFTs (set when displays are chosen)
// -----------------------------------------------------------------------------
#define EVE_TFT_SPI_MOSI (-1)
#define EVE_TFT_SPI_SCK (-1)
#define EVE_TFT_SPI_MISO (-1)
#define EVE_TFT_LEFT_CS (-1)
#define EVE_TFT_RIGHT_CS (-1)
#define EVE_TFT_DC (-1)
#define EVE_TFT_RST (-1)

// -----------------------------------------------------------------------------
// I2C for dual ToF (VL53L1X or similar)
// -----------------------------------------------------------------------------
#define EVE_I2C_SDA (-1)
#define EVE_I2C_SCL (-1)

// -----------------------------------------------------------------------------
// Servos (LEDC PWM), NeoPixel data
// -----------------------------------------------------------------------------
#define EVE_SERVO_L_PIN (-1)
#define EVE_SERVO_R_PIN (-1)
#define EVE_NEOPIXEL_PIN (-1)
#define EVE_NEOPIXEL_COUNT 8

// -----------------------------------------------------------------------------
// Battery / power monitoring (ACS712 current sensor + voltage divider)
// Set EVE_ENABLE_POWER_MONITOR to 1 once sensors are wired up.
// Change the ADC pin numbers below to match your PCB layout.
// -----------------------------------------------------------------------------
/** Set to 1 to enable power monitoring (ADC reads + battery state machine). */
#define EVE_ENABLE_POWER_MONITOR 1

/** ADC pin for battery voltage divider output (GPIO number). */
#define EVE_BAT_VOLT_ADC_PIN  4

/** ADC pin for ACS712 current sensor output (GPIO number). */
#define EVE_BAT_CURR_ADC_PIN  5

/**
 * Optional digital input for a charger-present signal (e.g. from charging dock).
 * Set to -1 if not wired — charging detection falls back to current direction only.
 */
#define EVE_CHARGER_PRESENT_PIN (-1)

// -----------------------------------------------------------------------------
// Feature enables — turn on only after hardware matches pins above
// -----------------------------------------------------------------------------
#define EVE_ENABLE_EYES 0
#define EVE_ENABLE_TOF 0
#define EVE_ENABLE_SERVOS 0
#define EVE_ENABLE_NEOPIXEL 0
#define EVE_ENABLE_AUDIO 0

// -----------------------------------------------------------------------------
// Protocol timing
// -----------------------------------------------------------------------------
static const uint32_t EVE_HELLO_RETRY_MS = 500;
static const uint32_t EVE_HEARTBEAT_MS = 250;
/** No inbound UART frame for this long in strict modes (ESCORT/INTERACT) → link lost */
static const uint32_t EVE_LINK_LOST_MS = 8000;
/** How often EVE sends a power-status packet to WALL-E (ms). */
static const uint32_t EVE_POWER_STATUS_INTERVAL_MS = 500;
