#pragma once

// ============================================================
// WALL-E Base Brain — ESP32-S3 production GPIO map
//
// Target: ESP32-S3 Dev Module / DevKitC-1 class board.
//
// Design goals:
//   * Native ESP-NOW on the S3 — no external radio gateway.
//   * Avoid GPIO26..37 because they may be occupied by flash/PSRAM.
//   * Avoid GPIO19/20 so USB-JTAG / native USB remains available.
//   * Avoid UART0 GPIO43/44 so serial diagnostics remain available.
//   * Avoid strapping GPIO0/3/45/46 for external robot hardware.
//   * Drop the old local ST7789 Base display. CYD/LROS remain the operator UI,
//     which frees five clean GPIOs for WALL-E hardware.
// ============================================================

#include <Arduino.h>

#define WALLE_BASE_BOARD_NAME "ESP32-S3 Base Brain"
#define WALLE_BASE_LOCAL_TFT  0

// ------------------------------------------------------------
// Drive — L298N dual H-bridge
// ------------------------------------------------------------
#define BASE_PIN_MOTOR_LEFT_IN1    4
#define BASE_PIN_MOTOR_LEFT_IN2    5
#define BASE_PIN_MOTOR_LEFT_ENA    6
#define BASE_PIN_MOTOR_RIGHT_IN3   7
#define BASE_PIN_MOTOR_RIGHT_IN4   8
#define BASE_PIN_MOTOR_RIGHT_ENB   9

// ------------------------------------------------------------
// Shared I2C — PCA9685, MPU6050, compass, VL53L1X
// ------------------------------------------------------------
#define BASE_PIN_I2C_SDA          17
#define BASE_PIN_I2C_SCL          18

// ------------------------------------------------------------
// Analog monitoring
// ADC1-capable pins are used so Wi-Fi/ESP-NOW operation does not interfere
// with readings.
// ------------------------------------------------------------
#define BASE_PIN_BATTERY_ADC       1
#define BASE_PIN_CURRENT_ADC       2
#define BASE_PIN_LDR_ADC          10

// ------------------------------------------------------------
// Effects
// ------------------------------------------------------------
#define BASE_PIN_FLASHLIGHT       11
#define BASE_PIN_LASER            12

#define WALLE_BASE_FLASHLIGHT_ON_PCA9685 0
#define BASE_FLASHLIGHT_PCA_CHANNEL      -1

// ------------------------------------------------------------
// Ultrasonic range sensor
// ------------------------------------------------------------
#define BASE_PIN_SONAR_TRIGGER    13
#define BASE_PIN_SONAR_ECHO       14

// ------------------------------------------------------------
// Dock IR transmitters
// ------------------------------------------------------------
#define BASE_PIN_DOCK_IR_LEFT     15
#define BASE_PIN_DOCK_IR_RIGHT    16

// ------------------------------------------------------------
// GPS UART2 — NMEA receive only
// GPS TX -> S3 GPIO21. A Base->GPS TX wire is not required.
// ------------------------------------------------------------
#define BASE_PIN_GPS_RX           21
#define BASE_PIN_GPS_TX           -1

// ------------------------------------------------------------
// Four obstacle sensors — direct GPIO inputs
// ------------------------------------------------------------
#define WALLE_BASE_OBSTACLES_PCF8574 0
#define BASE_OBS_PCF8574_ADDR        0x20
#define BASE_OBS_PCF8574_FRONT_L_BIT 0
#define BASE_OBS_PCF8574_FRONT_R_BIT 1
#define BASE_OBS_PCF8574_REAR_L_BIT  2
#define BASE_OBS_PCF8574_REAR_R_BIT  3

#define BASE_PIN_OBS_FRONT_L      38
#define BASE_PIN_OBS_FRONT_R      39
#define BASE_PIN_OBS_REAR_L       40
#define BASE_PIN_OBS_REAR_R       41

// ------------------------------------------------------------
// Old local ST7789 Base display — intentionally not fitted on S3 production.
// ------------------------------------------------------------
#define BASE_PIN_TFT_MOSI        -1
#define BASE_PIN_TFT_SCK         -1
#define BASE_PIN_TFT_DC          -1
#define BASE_PIN_TFT_RST         -1
#define BASE_PIN_TFT_BL          -1
