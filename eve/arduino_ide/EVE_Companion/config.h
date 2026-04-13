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
// Layout: sensor on EVE's physical LEFT looks left; RIGHT looks right.
// Optional center: third sensor or fused "forward" — set EVE_TOF_HAS_CENTER 0/1
// -----------------------------------------------------------------------------
#define EVE_I2C_SDA (-1)
#define EVE_I2C_SCL (-1)
#define EVE_TOF_HAS_CENTER 0
/** GPIO tied to second VL53L1X XSHUT (LOW = sensor off). -1 = single forward sensor only. */
#define EVE_TOF_XSHUT_SECOND (-1)
/** I2C addresses after init (first moved off 0x29, second stays 0x29) */
#define EVE_TOF_ADDR_FIRST 0x30
#define EVE_TOF_ADDR_SECOND 0x29
/** "Noticed" range — beyond = ignore for awareness */
#define EVE_TOF_NEAR_MM 850
#define EVE_TOF_FAR_IGNORE_MM 2400
/** Poll period when ToF enabled */
#define EVE_TOF_POLL_MS 40u
/** 1 = synthetic motion for bench (no VL53L1X library needed) */
#define EVE_TOF_SIMULATE 0

// -----------------------------------------------------------------------------
// Servos (LEDC PWM), NeoPixel data
// -----------------------------------------------------------------------------
#define EVE_SERVO_L_PIN (-1)
#define EVE_SERVO_R_PIN (-1)
#define EVE_NEOPIXEL_PIN (-1)
#define EVE_NEOPIXEL_COUNT 8

// -----------------------------------------------------------------------------
// Battery: voltage divider on pack (or 5 V rail sense) + optional inline current sensor
// Use ADC1-capable GPIOs (not 17/18 UART). Set pin to -1 to disable that channel.
// Default: 4 = voltage, 5 = current — adjust to match your EVE PCB.
// Divider: BAT+ -- R1 --+-- ADC pin -- R2 -- GND  (same topology as WALL-E base)
// -----------------------------------------------------------------------------
#define EVE_BAT_ADC_PIN 4
#define EVE_CUR_ADC_PIN 5
#define EVE_ENABLE_BATTERY_MONITOR 1

#define EVE_BAT_R1 10000
#define EVE_BAT_R2 10000
#define EVE_BAT_MIN_V 3.25f
#define EVE_BAT_MAX_V 4.20f
#define EVE_BAT_CALIB 1.0f

#define EVE_CUR_ZERO_V 1.65f
#define EVE_CUR_SENSITIVITY_V_PER_A 0.122f

#define EVE_BAT_SAMPLES 12
#define EVE_CUR_SAMPLES 8
#define EVE_BAT_POLL_MS 2000u

#define EVE_BAT_WARN_PCT 25
#define EVE_BAT_CRIT_PCT 15
#define EVE_BAT_LOW_REPORT_MIN_MS 20000u

// -----------------------------------------------------------------------------
// EVE face (LVGL 9) — single logical panel; set resolution to match your TFT
// When TFT pins are unset (<0), firmware runs a stub flush (LVGL works, no pixels).
// -----------------------------------------------------------------------------
#define EVE_FACE_LCD_HOR_RES 240
#define EVE_FACE_LCD_VER_RES 280
/** Partial buffer height (lines); larger = smoother, more RAM */
#define EVE_FACE_LVGL_BUF_LINES 40
/** Serial bench: type 0–9 / n p s h c a l r u d w / ? for help */
#define EVE_FACE_DEBUG_BENCH 1
#define EVE_FACE_TFT_HAS_PIN(p) ((p) >= 0)
#define EVE_FACE_GFX_READY                                                                 \
  (EVE_FACE_TFT_HAS_PIN(EVE_TFT_SPI_MOSI) && EVE_FACE_TFT_HAS_PIN(EVE_TFT_SPI_SCK) &&      \
   EVE_FACE_TFT_HAS_PIN(EVE_TFT_LEFT_CS) && EVE_FACE_TFT_HAS_PIN(EVE_TFT_DC))

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
