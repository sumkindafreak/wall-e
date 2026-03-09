#pragma once

// ============================================================
//  WALL-E IMU Manager
//  MPU6050 via I2C (shares bus with PCA9685)
//
//  REQUIRED LIBRARY: "MPU6050" by Electronic Cats
//                    OR "Adafruit MPU6050" — both work,
//                    change the include in imu_manager.cpp
//                    if needed.
// ============================================================

#include <Arduino.h>

// MPU6050 I2C address (AD0 low = 0x68, AD0 high = 0x69)
#define MPU6050_ADDR    0x68

// Tilt alert threshold in degrees
#define TILT_ALERT_DEG  25.0f

// How often to read the IMU (ms)
#define IMU_POLL_MS     50    // 20Hz

struct ImuData {
  float pitch;      // forward/back tilt  (degrees)
  float roll;       // left/right tilt    (degrees)
  float accelX;     // m/s^2 (offset-corrected)
  float accelY;
  float accelZ;
  bool  tiltAlert;  // true if tilt exceeds threshold
  bool  valid;      // false if IMU not responding
};

void    imuInit();           // Legacy: calls beginIMU()
void    imuHandle();         // Legacy: calls updateIMU()
void    beginIMU();         // Start IMU and begin auto-calibration (non-blocking)
void    updateIMU();         // Call every loop: runs calibration or normal read
void    calibrateIMU();      // Manually start calibration (resets and begins)
bool    isIMUCalibrated();   // true when calibration done and data is valid
void    forceRecalibration(); // Reset and restart calibration (for /imu/recalibrate)
const ImuData& imuGetData();
String  imuGetStatusJSON();
