#pragma once

// ============================================================
// WALL-E Base Brain — board-level GPIO map
//
// ESP32-P4 primary target:
//   Waveshare ESP32-P4-Module-DEV-KIT 40-pin header.
//
// IMPORTANT P4 choices:
//   * I2C uses the board's documented SDA=GPIO7 / SCL=GPIO8.
//   * GPIO24/25 are repurposed from the P4 USB-FS pair for the dedicated
//     radio-gateway UART. Use the board's CH343 UART Type-C (GPIO37/38)
//     for flashing / Serial diagnostics instead of native USB-FS CDC.
//   * GPIO37/38 are intentionally left untouched for the onboard CH343.
//   * GPIO53 is intentionally left untouched because the board uses it for
//     the onboard audio power-amplifier enable.
//   * The legacy SPI ST7789 Base display is disabled on P4. CYD/LROS remain
//     operator displays; a P4 DSI display can be added separately later.
//
// Keep all Base hardware definitions here. Driver headers should alias these
// constants rather than inventing their own GPIO numbers.
// ============================================================

#include <Arduino.h>

#if defined(CONFIG_IDF_TARGET_ESP32P4)

#define WALLE_BASE_BOARD_NAME "Waveshare ESP32-P4-Module-DEV-KIT"
#define WALLE_BASE_LOCAL_TFT  0

// ------------------------------------------------------------
// Drive — L298N dual H-bridge
// ------------------------------------------------------------
#define BASE_PIN_MOTOR_LEFT_IN1   0
#define BASE_PIN_MOTOR_LEFT_IN2   1
#define BASE_PIN_MOTOR_LEFT_ENA   2
#define BASE_PIN_FLASHLIGHT       3
#define BASE_PIN_MOTOR_RIGHT_IN3  4
#define BASE_PIN_MOTOR_RIGHT_IN4  5
#define BASE_PIN_MOTOR_RIGHT_ENB  6

// ------------------------------------------------------------
// Shared I2C — PCA9685, MPU6050, compass, VL53L1X
// ------------------------------------------------------------
#define BASE_PIN_I2C_SDA          7
#define BASE_PIN_I2C_SCL          8

// ------------------------------------------------------------
// Analog monitoring (ESP32-P4 ADC1-capable exposed pins)
// ------------------------------------------------------------
#define BASE_PIN_BATTERY_ADC     20
#define BASE_PIN_CURRENT_ADC     21
#define BASE_PIN_LDR_ADC         22

// ------------------------------------------------------------
// Effects / local sensors
// ------------------------------------------------------------
#define BASE_PIN_LASER           23
#define BASE_PIN_SONAR_TRIGGER   26
#define BASE_PIN_SONAR_ECHO      27

// ------------------------------------------------------------
// Dock IR transmitters
// ------------------------------------------------------------
#define BASE_PIN_DOCK_IR_LEFT    32
#define BASE_PIN_DOCK_IR_RIGHT   33

// ------------------------------------------------------------
// GPS UART
// GPIO36 is a strapping-related pin on the module, so it is used only as
// WALL-E's TX output. Nothing external should drive it during reset.
// ------------------------------------------------------------
#define BASE_PIN_GPS_TX          36
#define BASE_PIN_GPS_RX          54

// ------------------------------------------------------------
// Four obstacle sensors — inputs only
// ------------------------------------------------------------
#define BASE_PIN_OBS_FRONT_L     45
#define BASE_PIN_OBS_FRONT_R     46
#define BASE_PIN_OBS_REAR_L      47
#define BASE_PIN_OBS_REAR_R      48

// ------------------------------------------------------------
// P4 <-> ESP32 radio gateway UART
// These are the module's USB1 full-speed GPIO pair repurposed as UART.
// Native USB-FS on this pair is therefore unavailable in this build.
// ------------------------------------------------------------
#define BASE_PIN_RADIO_UART_RX   24
#define BASE_PIN_RADIO_UART_TX   25

// ------------------------------------------------------------
// Board-reserved pins documented here so they are not accidentally reused.
// ------------------------------------------------------------
#define BASE_PIN_DEBUG_UART_TX   37
#define BASE_PIN_DEBUG_UART_RX   38
#define BASE_PIN_AUDIO_PA_ENABLE 53

// ------------------------------------------------------------
// Compile-time collision guard for all WALL-E-assigned P4 pins.
// Negative pin values are ignored so future optional devices can use -1.
// ------------------------------------------------------------
namespace walle_base_pincheck {
constexpr int kAssignedPins[] = {
    BASE_PIN_MOTOR_LEFT_IN1,
    BASE_PIN_MOTOR_LEFT_IN2,
    BASE_PIN_MOTOR_LEFT_ENA,
    BASE_PIN_FLASHLIGHT,
    BASE_PIN_MOTOR_RIGHT_IN3,
    BASE_PIN_MOTOR_RIGHT_IN4,
    BASE_PIN_MOTOR_RIGHT_ENB,
    BASE_PIN_I2C_SDA,
    BASE_PIN_I2C_SCL,
    BASE_PIN_BATTERY_ADC,
    BASE_PIN_CURRENT_ADC,
    BASE_PIN_LDR_ADC,
    BASE_PIN_LASER,
    BASE_PIN_SONAR_TRIGGER,
    BASE_PIN_SONAR_ECHO,
    BASE_PIN_DOCK_IR_LEFT,
    BASE_PIN_DOCK_IR_RIGHT,
    BASE_PIN_GPS_TX,
    BASE_PIN_GPS_RX,
    BASE_PIN_OBS_FRONT_L,
    BASE_PIN_OBS_FRONT_R,
    BASE_PIN_OBS_REAR_L,
    BASE_PIN_OBS_REAR_R,
    BASE_PIN_RADIO_UART_RX,
    BASE_PIN_RADIO_UART_TX,
};

constexpr bool allUnique() {
  for (size_t i = 0; i < (sizeof(kAssignedPins) / sizeof(kAssignedPins[0])); ++i) {
    if (kAssignedPins[i] < 0) continue;
    for (size_t j = i + 1; j < (sizeof(kAssignedPins) / sizeof(kAssignedPins[0])); ++j) {
      if (kAssignedPins[j] < 0) continue;
      if (kAssignedPins[i] == kAssignedPins[j]) return false;
    }
  }
  return true;
}

constexpr bool avoidsBoardReservedPins() {
  for (size_t i = 0; i < (sizeof(kAssignedPins) / sizeof(kAssignedPins[0])); ++i) {
    const int p = kAssignedPins[i];
    if (p < 0) continue;
    if (p == BASE_PIN_DEBUG_UART_TX ||
        p == BASE_PIN_DEBUG_UART_RX ||
        p == BASE_PIN_AUDIO_PA_ENABLE) {
      return false;
    }
  }
  return true;
}
}  // namespace walle_base_pincheck

static_assert(walle_base_pincheck::allUnique(),
              "WALL-E P4 GPIO collision: two Base functions share a pin");
static_assert(walle_base_pincheck::avoidsBoardReservedPins(),
              "WALL-E P4 GPIO map uses a board-reserved pin");

#else

// ============================================================
// Legacy ESP32-S3 regression map
//
// Retained to keep the existing S3 regression build useful while P4 becomes
// the production Base. It intentionally mirrors the established hardware
// definitions instead of silently changing an already-wired S3 robot.
// ============================================================
#define WALLE_BASE_BOARD_NAME "ESP32-S3 legacy Base"
#define WALLE_BASE_LOCAL_TFT  1

#define BASE_PIN_MOTOR_LEFT_IN1   4
#define BASE_PIN_MOTOR_LEFT_IN2   5
#define BASE_PIN_MOTOR_LEFT_ENA   6
#define BASE_PIN_MOTOR_RIGHT_IN3  7
#define BASE_PIN_MOTOR_RIGHT_IN4  8
#define BASE_PIN_MOTOR_RIGHT_ENB  9

#define BASE_PIN_I2C_SDA         21
#define BASE_PIN_I2C_SCL         17

#define BASE_PIN_BATTERY_ADC     19
#define BASE_PIN_CURRENT_ADC      2
#define BASE_PIN_LDR_ADC          3
#define BASE_PIN_FLASHLIGHT      10
#define BASE_PIN_LASER           18
#define BASE_PIN_SONAR_TRIGGER   26
#define BASE_PIN_SONAR_ECHO      27

#define BASE_PIN_DOCK_IR_LEFT    21
#define BASE_PIN_DOCK_IR_RIGHT   38

#define BASE_PIN_GPS_RX          16
#define BASE_PIN_GPS_TX          17

#define BASE_PIN_OBS_FRONT_L     22
#define BASE_PIN_OBS_FRONT_R     23
#define BASE_PIN_OBS_REAR_L      20
#define BASE_PIN_OBS_REAR_R      47

// Native ESP-NOW build does not use the UART gateway.
#define BASE_PIN_RADIO_UART_RX   -1
#define BASE_PIN_RADIO_UART_TX   -1

// Legacy external ST7789 pins.
#define BASE_PIN_TFT_MOSI        11
#define BASE_PIN_TFT_SCK         12
#define BASE_PIN_TFT_DC          13
#define BASE_PIN_TFT_RST         14
#define BASE_PIN_TFT_BL          15

#endif
