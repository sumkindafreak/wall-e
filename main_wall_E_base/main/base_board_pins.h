#pragma once

// ============================================================
// WALL-E Base Brain — ESP32-S3 board-level GPIO map
//
// Production MCU: ESP32-S3 Dev Module / DevKitC-1 class board.
//
// IMPORTANT:
// These assignments preserve WALL-E's existing legacy Base wiring so this
// architecture switch does not silently move physical connections. A separate
// S3 GPIO audit is required before the final bench loom is locked, because
// some old assignments pre-date the current S3 target.
// ============================================================

#include <Arduino.h>

#define WALLE_BASE_BOARD_NAME "ESP32-S3 Base Brain"
#define WALLE_BASE_LOCAL_TFT  1

// ------------------------------------------------------------
// Drive — L298N dual H-bridge
// ------------------------------------------------------------
#define BASE_PIN_MOTOR_LEFT_IN1   4
#define BASE_PIN_MOTOR_LEFT_IN2   5
#define BASE_PIN_MOTOR_LEFT_ENA   6
#define BASE_PIN_MOTOR_RIGHT_IN3  7
#define BASE_PIN_MOTOR_RIGHT_IN4  8
#define BASE_PIN_MOTOR_RIGHT_ENB  9

// ------------------------------------------------------------
// Shared I2C — PCA9685, MPU6050, compass, VL53L1X
// ------------------------------------------------------------
#define BASE_PIN_I2C_SDA         21
#define BASE_PIN_I2C_SCL         17

// ------------------------------------------------------------
// Analog monitoring / effects
// ------------------------------------------------------------
#define BASE_PIN_BATTERY_ADC     19
#define BASE_PIN_CURRENT_ADC      2
#define BASE_PIN_LDR_ADC          3
#define BASE_PIN_FLASHLIGHT      10
#define BASE_PIN_LASER           18

// S3 keeps the original direct flashlight GPIO path.
#define WALLE_BASE_FLASHLIGHT_ON_PCA9685 0
#define BASE_FLASHLIGHT_PCA_CHANNEL      -1

// ------------------------------------------------------------
// Range sensing
// ------------------------------------------------------------
#define BASE_PIN_SONAR_TRIGGER   26
#define BASE_PIN_SONAR_ECHO      27

// ------------------------------------------------------------
// Dock IR transmitters
// ------------------------------------------------------------
#define BASE_PIN_DOCK_IR_LEFT    21
#define BASE_PIN_DOCK_IR_RIGHT   38

// ------------------------------------------------------------
// GPS UART2
// ------------------------------------------------------------
#define BASE_PIN_GPS_RX          16
#define BASE_PIN_GPS_TX          17

// ------------------------------------------------------------
// Four obstacle sensors — legacy direct-GPIO arrangement
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// Legacy external ST7789 display
// ------------------------------------------------------------
#define BASE_PIN_TFT_MOSI        11
#define BASE_PIN_TFT_SCK         12
#define BASE_PIN_TFT_DC          13
#define BASE_PIN_TFT_RST         14
#define BASE_PIN_TFT_BL          15
