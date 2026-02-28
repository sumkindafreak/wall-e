// ============================================================
//  WALL-E IMU Manager Implementation
//  MPU6050 — pitch/roll tilt, auto-calibration on boot
//  Shares I2C bus with PCA9685 (Wire already begun in servo_manager)
// ============================================================

#include "imu_manager.h"
#include <Wire.h>
#include <MPU6050.h>
#include <Arduino.h>

static MPU6050 mpu;
static ImuData _data = {};
static unsigned long _lastRead = 0;

// --- Calibration state (all in imu_manager) ---
static bool imuCalibrated   = false;
static bool calibrating     = false;
static unsigned long calibrationStartTime = 0;
static float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;
static float accelOffsetX = 0, accelOffsetY = 0, accelOffsetZ = 0;

// Running sums for calibration (physical units)
static float sumGx = 0, sumGy = 0, sumGz = 0;
static float sumAx = 0, sumAy = 0, sumAz = 0;
static uint32_t calCount = 0;
static float prevAx = 0, prevAy = 0, prevAz = 0;
static bool prevAccelValid = false;
static unsigned long movementFirstSeen = 0;  // debounce: restart only after sustained movement

// Scale factors (MPU6050 ±250 deg/s → 131 LSB/(deg/s); ±2g → 16384 LSB/g)
#define GYRO_LSB_PER_DEG_S   131.0f
#define ACCEL_LSB_PER_G      16384.0f
#define CALIBRATION_DURATION_MS  3000UL
// Movement thresholds: high enough to ignore sensor noise, low enough to catch real motion
#define GYRO_MOVE_THRESH_RAW     1200    // LSB (~9 deg/s) — bias/noise often 200–800
#define ACCEL_MOVE_THRESH_G      0.15f   // g — delta from previous sample (noise ~0.02–0.05)
#define MOVEMENT_DEBOUNCE_MS     150UL   // require sustained movement before restart

static void clearCalibrationSums() {
  sumGx = sumGy = sumGz = 0;
  sumAx = sumAy = sumAz = 0;
  calCount = 0;
  prevAccelValid = false;
  movementFirstSeen = 0;
}

// ============================================================
//  beginIMU — start IMU and begin auto-calibration
// ============================================================
void beginIMU() {
  mpu.initialize();
  if (!mpu.testConnection()) {
    _data.valid = false;
    imuCalibrated = false;
    calibrating = false;
    Serial.println("[IMU] MPU6050 not found — check wiring and I2C address");
    return;
  }
  _data.valid = true;
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  mpu.setDLPFMode(MPU6050_DLPF_BW_20);
  Serial.printf("[IMU] MPU6050 found at 0x%02X\n", MPU6050_ADDR);
  calibrateIMU();
}

// ============================================================
//  calibrateIMU — start (or restart) calibration
// ============================================================
void calibrateIMU() {
  imuCalibrated = false;
  calibrating = true;
  calibrationStartTime = millis();
  clearCalibrationSums();
  gyroOffsetX = gyroOffsetY = gyroOffsetZ = 0;
  accelOffsetX = accelOffsetY = accelOffsetZ = 0;
  Serial.println("[IMU] Starting auto calibration...");
}

void forceRecalibration() {
  calibrateIMU();
}

// ============================================================
//  updateIMU — non-blocking: calibration or normal read
// ============================================================
void updateIMU() {
  if (!_data.valid) return;

  if (calibrating) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float axG = ax / ACCEL_LSB_PER_G;
    float ayG = ay / ACCEL_LSB_PER_G;
    float azG = az / ACCEL_LSB_PER_G;

    // Movement detection: gyro above threshold or accel jump (thresholds set to ignore noise)
    bool movement = (abs(gx) > GYRO_MOVE_THRESH_RAW) ||
                    (abs(gy) > GYRO_MOVE_THRESH_RAW) ||
                    (abs(gz) > GYRO_MOVE_THRESH_RAW);
    if (prevAccelValid) {
      if (fabsf(axG - prevAx) > ACCEL_MOVE_THRESH_G ||
          fabsf(ayG - prevAy) > ACCEL_MOVE_THRESH_G ||
          fabsf(azG - prevAz) > ACCEL_MOVE_THRESH_G)
        movement = true;
    }
    prevAx = axG; prevAy = ayG; prevAz = azG;
    prevAccelValid = true;

    if (movement) {
      if (movementFirstSeen == 0)
        movementFirstSeen = millis();
      else if ((millis() - movementFirstSeen) >= MOVEMENT_DEBOUNCE_MS) {
        Serial.println("[IMU] Movement detected - restarting calibration");
        calibrationStartTime = millis();
        clearCalibrationSums();
        return;
      }
    } else {
      movementFirstSeen = 0;
    }

    sumGx += gx / GYRO_LSB_PER_DEG_S;
    sumGy += gy / GYRO_LSB_PER_DEG_S;
    sumGz += gz / GYRO_LSB_PER_DEG_S;
    sumAx += axG * 9.81f;
    sumAy += ayG * 9.81f;
    sumAz += azG * 9.81f;
    calCount++;

    if ((millis() - calibrationStartTime) >= CALIBRATION_DURATION_MS && calCount > 0) {
      gyroOffsetX = sumGx / calCount;
      gyroOffsetY = sumGy / calCount;
      gyroOffsetZ = sumGz / calCount;
      accelOffsetX = sumAx / calCount;
      accelOffsetY = sumAy / calCount;
      accelOffsetZ = sumAz / calCount;
      imuCalibrated = true;
      calibrating = false;
      Serial.println("[IMU] Calibration complete");
      Serial.printf("[IMU] Gyro offsets:  X: %.3f  Y: %.3f  Z: %.3f (deg/s)\n",
        gyroOffsetX, gyroOffsetY, gyroOffsetZ);
      Serial.printf("[IMU] Accel offsets: X: %.3f  Y: %.3f  Z: %.3f (m/s^2)\n",
        accelOffsetX, accelOffsetY, accelOffsetZ);
    }
    return;
  }

  // Normal operation: rate-limit and apply offsets
  if ((millis() - _lastRead) < IMU_POLL_MS) return;
  _lastRead = millis();

  if (!imuCalibrated) return;

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float gxDeg = gx / GYRO_LSB_PER_DEG_S - gyroOffsetX;
  float gyDeg = gy / GYRO_LSB_PER_DEG_S - gyroOffsetY;
  float gzDeg = gz / GYRO_LSB_PER_DEG_S - gyroOffsetZ;
  (void)gxDeg; (void)gyDeg; (void)gzDeg; // reserved for future use

  float axG = ax / ACCEL_LSB_PER_G;
  float ayG = ay / ACCEL_LSB_PER_G;
  float azG = az / ACCEL_LSB_PER_G;
  _data.accelX = axG * 9.81f - accelOffsetX;
  _data.accelY = ayG * 9.81f - accelOffsetY;
  _data.accelZ = azG * 9.81f - accelOffsetZ;

  float axGc = _data.accelX / 9.81f;
  float ayGc = _data.accelY / 9.81f;
  float azGc = _data.accelZ / 9.81f;
  _data.pitch = atan2f(ayGc, sqrtf(axGc * axGc + azGc * azGc)) * 180.0f / PI;
  _data.roll  = atan2f(-axGc, azGc) * 180.0f / PI;
  _data.tiltAlert = (fabsf(_data.pitch) > TILT_ALERT_DEG ||
                     fabsf(_data.roll)  > TILT_ALERT_DEG);
}

// ============================================================
//  Legacy and accessors
// ============================================================
void imuInit() {
  beginIMU();
}

void imuHandle() {
  updateIMU();
}

bool isIMUCalibrated() {
  return imuCalibrated;
}

const ImuData& imuGetData() {
  return _data;
}

String imuGetStatusJSON() {
  String s = "{";
  s += "\"valid\":"      + String(_data.valid ? "true" : "false") + ",";
  s += "\"calibrated\":" + String(imuCalibrated ? "true" : "false") + ",";
  s += "\"pitch\":"      + String(_data.pitch, 1) + ",";
  s += "\"roll\":"       + String(_data.roll,  1) + ",";
  s += "\"tiltAlert\":"  + String(_data.tiltAlert ? "true" : "false");
  s += "}";
  return s;
}
