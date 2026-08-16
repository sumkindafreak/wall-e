#pragma once

// ============================================================
// WALL-E Base Brain — board-level GPIO map
//
// ESP32-P4 primary target:
//   Waveshare ESP32-P4-Module-DEV-KIT 40-pin header.
//
// P4 design rules:
//   * I2C uses the board's SDA=GPIO7 / SCL=GPIO8.
//   * GPIO24/25 are repurposed from USB-FS for the radio-gateway UART.
//     Flash/debug through the CH343 UART Type-C on GPIO37/38.
//   * GPIO54 is reserved for the onboard C6 ESP-Hosted reset path.
//   * GPIO53 is reserved for the onboard speaker amplifier enable.
//   * GPIO39..48 are kept out of WALL-E's 3.3 V peripheral map. They belong
//     to the P4's separately powered I/O domain and several are used by the
//     onboard microSD circuitry.
//   * GPIO36 is a boot/strapping-related pin and is left unused externally.
//   * GPS is RX-only on GPIO3. Standard NMEA operation does not require a
//     Base->GPS TX wire.
//   * The four obstacle sensors use a PCF8574 I2C input expander at 0x20.
//   * Flashlight MOSFET is driven from spare PCA9685 channel 9.
//   * The legacy SPI ST7789 Base display is disabled on P4. CYD/LROS remain
//     operator displays; a P4 DSI display can be added later.
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
#define BASE_PIN_MOTOR_RIGHT_IN3  4
#define BASE_PIN_MOTOR_RIGHT_IN4  5
#define BASE_PIN_MOTOR_RIGHT_ENB  6

// ------------------------------------------------------------
// Shared I2C — PCA9685, MPU6050, compass, VL53L1X, PCF8574
// ------------------------------------------------------------
#define BASE_PIN_I2C_SDA          7
#define BASE_PIN_I2C_SCL          8

// ------------------------------------------------------------
// Analog monitoring (exposed ADC1-capable GPIO)
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

// Flashlight uses unused PCA9685 output 9 rather than a scarce P4 GPIO.
#define WALLE_BASE_FLASHLIGHT_ON_PCA9685  1
#define BASE_FLASHLIGHT_PCA_CHANNEL       9
#define BASE_PIN_FLASHLIGHT              -1

// ------------------------------------------------------------
// Dock IR transmitters
// ------------------------------------------------------------
#define BASE_PIN_DOCK_IR_LEFT    32
#define BASE_PIN_DOCK_IR_RIGHT   33

// ------------------------------------------------------------
// GPS UART2 — NMEA receive only
// ------------------------------------------------------------
#define BASE_PIN_GPS_RX           3
#define BASE_PIN_GPS_TX          -1

// ------------------------------------------------------------
// Four obstacle sensors — PCF8574 I2C expander
// 3.3 V PCF8574, address pins A0/A1/A2 low => 0x20.
// Sensor outputs connect to P0..P3. Inputs are released HIGH internally by
// writing ones to the PCF8574 port byte before reads.
// ------------------------------------------------------------
#define WALLE_BASE_OBSTACLES_PCF8574  1
#define BASE_OBS_PCF8574_ADDR          0x20
#define BASE_OBS_PCF8574_FRONT_L_BIT   0
#define BASE_OBS_PCF8574_FRONT_R_BIT   1
#define BASE_OBS_PCF8574_REAR_L_BIT    2
#define BASE_OBS_PCF8574_REAR_R_BIT    3

#define BASE_PIN_OBS_FRONT_L          -1
#define BASE_PIN_OBS_FRONT_R          -1
#define BASE_PIN_OBS_REAR_L           -1
#define BASE_PIN_OBS_REAR_R           -1

// ------------------------------------------------------------
// P4 <-> ESP32 radio gateway UART1
// ------------------------------------------------------------
#define BASE_PIN_RADIO_UART_RX   24
#define BASE_PIN_RADIO_UART_TX   25

// ------------------------------------------------------------
// Board-reserved / deliberately avoided GPIO
// ------------------------------------------------------------
#define BASE_PIN_BOOT_ENABLE      36
#define BASE_PIN_DEBUG_UART_TX    37
#define BASE_PIN_DEBUG_UART_RX    38
#define BASE_PIN_AUDIO_PA_ENABLE  53
#define BASE_PIN_HOSTED_WIFI_RST  54

namespace walle_base_pincheck {
constexpr int kAssignedPins[] = {
    BASE_PIN_MOTOR_LEFT_IN1,
    BASE_PIN_MOTOR_LEFT_IN2,
    BASE_PIN_MOTOR_LEFT_ENA,
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
    if (p == BASE_PIN_BOOT_ENABLE ||
        p == BASE_PIN_DEBUG_UART_TX ||
        p == BASE_PIN_DEBUG_UART_RX ||
        p == BASE_PIN_AUDIO_PA_ENABLE ||
        p == BASE_PIN_HOSTED_WIFI_RST) {
      return false;
    }
  }
  return true;
}

constexpr bool avoidsLowVoltageDomain() {
  for (size_t i = 0; i < (sizeof(kAssignedPins) / sizeof(kAssignedPins[0])); ++i) {
    const int p = kAssignedPins[i];
    if (p >= 39 && p <= 48) return false;
  }
  return true;
}
}  // namespace walle_base_pincheck

static_assert(walle_base_pincheck::allUnique(),
              "WALL-E P4 GPIO collision: two Base functions share a pin");
static_assert(walle_base_pincheck::avoidsBoardReservedPins(),
              "WALL-E P4 GPIO map uses a reserved/strapping/hosted-WiFi pin");
static_assert(walle_base_pincheck::avoidsLowVoltageDomain(),
              "WALL-E P4 GPIO map uses GPIO39..48 low-voltage domain");

#else

// ============================================================
// Legacy ESP32-S3 regression map
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
#define WALLE_BASE_FLASHLIGHT_ON_PCA9685 0
#define BASE_FLASHLIGHT_PCA_CHANNEL      -1
#define BASE_PIN_LASER           18
#define BASE_PIN_SONAR_TRIGGER   26
#define BASE_PIN_SONAR_ECHO      27

#define BASE_PIN_DOCK_IR_LEFT    21
#define BASE_PIN_DOCK_IR_RIGHT   38

#define BASE_PIN_GPS_RX          16
#define BASE_PIN_GPS_TX          17

#define WALLE_BASE_OBSTACLES_PCF8574 0
#define BASE_OBS_PCF8574_ADDR        0x20
#define BASE_OBS_PCF8574_FRONT_L_BIT 0
#define BASE_OBS_PCF8574_FRONT_R_BIT 1
#define BASE_OBS_PCF8574_REAR_L_BIT  2
#define BASE_OBS_PCF8574_REAR_R_BIT  3
#define BASE_PIN_OBS_FRONT_L     22
#define BASE_PIN_OBS_FRONT_R     23
#define BASE_PIN_OBS_REAR_L      20
#define BASE_PIN_OBS_REAR_R      47

#define BASE_PIN_RADIO_UART_RX   -1
#define BASE_PIN_RADIO_UART_TX   -1

#define BASE_PIN_TFT_MOSI        11
#define BASE_PIN_TFT_SCK         12
#define BASE_PIN_TFT_DC          13
#define BASE_PIN_TFT_RST         14
#define BASE_PIN_TFT_BL          15

#endif
