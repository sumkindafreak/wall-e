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
// SD card (SPI) — primary asset storage for audio, config, graphics
// -----------------------------------------------------------------------------
#define EVE_SD_SPI_CS (-1)
#define EVE_SD_SPI_MOSI (-1)
#define EVE_SD_SPI_MISO (-1)
#define EVE_SD_SPI_SCK (-1)

// -----------------------------------------------------------------------------
// I2S audio output (speaker amplifier). Use port 1 if I2S mic uses port 0.
// Signal path: SD → WAV decoder → I2S → amplifier → speaker
// -----------------------------------------------------------------------------
#define EVE_I2S_BCLK_PIN (-1)
#define EVE_I2S_LRCK_PIN (-1)
#define EVE_I2S_DOUT_PIN (-1)
#define EVE_I2S_PORT_INDEX 1

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
// ESP32-S3 dev boards commonly expose the onboard addressable RGB LED on GPIO48.
// This is independent of the external EVE NeoPixel strip enable below.
#define EVE_ONBOARD_RGB_HEARTBEAT 1
#define EVE_ONBOARD_RGB_PIN 48
#define EVE_ONBOARD_RGB_BRIGHTNESS 24u

// -----------------------------------------------------------------------------
// Battery: INA219 I2C breakout (bus voltage + bidirectional current), OR legacy ADC
// INA219: wire per breakout (V+ to supply high side, V- toward load, same GND as ESP32).
// Library: PlatformIO lib_deps "adafruit/Adafruit INA219" + "adafruit/Adafruit BusIO".
// When EVE_BATTERY_INA219 is 1, EVE_BAT_ADC_PIN / EVE_CUR_ADC_PIN are ignored.
// If you also use ToF on I2C, set EVE_I2C_SDA/SCL to the same pins as below so one bus.
// -----------------------------------------------------------------------------
#define EVE_BATTERY_INA219 1
#define EVE_INA219_I2C_ADDR 0x40
#define EVE_INA219_SDA 8
#define EVE_INA219_SCL 9

/** Legacy voltage divider on ADC (only when EVE_BATTERY_INA219 is 0). R1 upper, R2 to GND. */
#define EVE_BAT_ADC_PIN 4
#define EVE_CUR_ADC_PIN 5
#define EVE_ENABLE_BATTERY_MONITOR 1

#define EVE_BAT_R1 10000
#define EVE_BAT_R2 10000
/** 2S Li-ion / LiPo (~7.4 V nominal on pack). Tune if you use NiMH or different chemistry. */
#define EVE_BAT_MIN_V 6.0f
#define EVE_BAT_MAX_V 8.4f
#define EVE_BAT_CALIB 1.0f

/** Legacy hall/ACS analog current sense (only when EVE_BATTERY_INA219 is 0). */
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

// Docked WebUI: EVE hosts a local AP only while on her dock.
#define EVE_ENABLE_DOCKED_WEBUI 1
#define EVE_WEBUI_AP_SSID "EVE-Desk"
#define EVE_WEBUI_AP_PASS "evefriend"
#define EVE_WEBUI_AP_CHANNEL 6
#define EVE_WEBUI_AP_MAX_CLIENTS 2
// Small grace avoids AP flapping on brief UART jitter; it still shuts down after undock/link loss.
#define EVE_DOCKED_WEBUI_OFF_GRACE_MS 3000u
// Bench-only dock mimic. Keep off for real use. Serial Monitor also supports:
//   docktest on
//   docktest off
#define EVE_ENABLE_SERIAL_CONSOLE 1
// Keep normal Serial Monitor output quiet. Set to 1 only when debugging UART frames.
#define EVE_SERIAL_VERBOSE_LOGS 0
#define EVE_ENABLE_DOCK_MIMIC_TEST 1
#define EVE_DOCK_MIMIC_ON_BOOT 0
#define EVE_DOCK_MIMIC_FAKE_CHARGING 1

// -----------------------------------------------------------------------------
// Protocol timing
// -----------------------------------------------------------------------------
static const uint32_t EVE_HELLO_RETRY_MS = 500;
static const uint32_t EVE_HEARTBEAT_MS = 250;
/** No inbound UART frame for this long in strict modes (ESCORT/INTERACT) → link lost */
static const uint32_t EVE_LINK_LOST_MS = 8000;
