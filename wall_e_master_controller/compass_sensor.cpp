// ============================================================
//  WALL-E Master Controller — Compass Implementation
// ============================================================

#include "compass_sensor.h"
#include <math.h>

// Internal state
static uint8_t s_compassType = COMPASS_HMC5883L;
static bool s_initialized = false;
static bool s_valid = false;
static float s_headingDeg = 0.0f;
static int16_t s_rawX = 0, s_rawY = 0, s_rawZ = 0;
static uint32_t s_lastUpdateMs = 0;

// Calibration data
static bool s_calibrating = false;
static int16_t s_minX = 32767, s_maxX = -32768;
static int16_t s_minY = 32767, s_maxY = -32768;
static int16_t s_minZ = 32767, s_maxZ = -32768;

// Low-pass filter
static float s_headingFiltered = 0.0f;
#define HEADING_FILTER_ALPHA  0.15f  // Smoothing factor

// HMC5883L registers
#define HMC_REG_CONFIG_A   0x00
#define HMC_REG_CONFIG_B   0x01
#define HMC_REG_MODE       0x02
#define HMC_REG_DATA_X_MSB 0x03

// QMC5883L registers
#define QMC_REG_DATA_X_LSB 0x00
#define QMC_REG_STATUS     0x06
#define QMC_REG_CONTROL1   0x09
#define QMC_REG_CONTROL2   0x0A
#define QMC_REG_PERIOD     0x0B

static bool detectCompassType() {
  // Try HMC5883L first
  Wire.beginTransmission(COMPASS_ADDR_HMC);
  if (Wire.endTransmission() == 0) {
    s_compassType = COMPASS_HMC5883L;
    Serial.println(F("[Compass] Detected HMC5883L"));
    return true;
  }
  
  // Try QMC5883L
  Wire.beginTransmission(COMPASS_ADDR_QMC);
  if (Wire.endTransmission() == 0) {
    s_compassType = COMPASS_QMC5883L;
    Serial.println(F("[Compass] Detected QMC5883L"));
    return true;
  }
  
  Serial.println(F("[Compass] ⚠️  No compass detected"));
  return false;
}

static bool initHMC5883L() {
  // Configure HMC5883L
  Wire.beginTransmission(COMPASS_ADDR_HMC);
  Wire.write(HMC_REG_CONFIG_A);
  Wire.write(0x70);  // 8 samples average, 15Hz output rate
  Wire.write(0x20);  // Gain = 1.3Ga
  Wire.write(0x00);  // Continuous measurement mode
  return (Wire.endTransmission() == 0);
}

static bool initQMC5883L() {
  // Soft reset QMC5883L
  Wire.beginTransmission(COMPASS_ADDR_QMC);
  Wire.write(QMC_REG_CONTROL2);
  Wire.write(0x80);
  Wire.endTransmission();
  delay(10);
  
  // Configure QMC5883L
  Wire.beginTransmission(COMPASS_ADDR_QMC);
  Wire.write(QMC_REG_CONTROL1);
  Wire.write(0x0D);  // Mode: continuous, ODR: 10Hz, Range: 2G, OSR: 512
  Wire.write(0x01);  // Period register
  return (Wire.endTransmission() == 0);
}

bool compassInit() {
  if (!detectCompassType()) {
    return false;
  }
  
  bool success = false;
  if (s_compassType == COMPASS_HMC5883L) {
    success = initHMC5883L();
  } else {
    success = initQMC5883L();
  }
  
  if (success) {
    s_initialized = true;
    Serial.printf("[Compass] Initialized %s\n", compassGetTypeName());
  } else {
    Serial.println(F("[Compass] ⚠️  Initialization failed"));
  }
  
  return success;
}

static void readHMC5883L() {
  Wire.beginTransmission(COMPASS_ADDR_HMC);
  Wire.write(HMC_REG_DATA_X_MSB);
  if (Wire.endTransmission() != 0) {
    s_valid = false;
    return;
  }
  
  Wire.requestFrom(COMPASS_ADDR_HMC, 6);
  if (Wire.available() >= 6) {
    s_rawX = (Wire.read() << 8) | Wire.read();
    s_rawZ = (Wire.read() << 8) | Wire.read();  // Z comes before Y on HMC
    s_rawY = (Wire.read() << 8) | Wire.read();
    s_valid = true;
  } else {
    s_valid = false;
  }
}

static void readQMC5883L() {
  Wire.beginTransmission(COMPASS_ADDR_QMC);
  Wire.write(QMC_REG_DATA_X_LSB);
  if (Wire.endTransmission() != 0) {
    s_valid = false;
    return;
  }
  
  Wire.requestFrom(COMPASS_ADDR_QMC, 6);
  if (Wire.available() >= 6) {
    s_rawX = Wire.read() | (Wire.read() << 8);
    s_rawY = Wire.read() | (Wire.read() << 8);
    s_rawZ = Wire.read() | (Wire.read() << 8);
    s_valid = true;
  } else {
    s_valid = false;
  }
}

void compassUpdate(uint32_t now) {
  if (!s_initialized) return;
  
  // Rate limit
  if (now - s_lastUpdateMs < COMPASS_UPDATE_MS) {
    return;
  }
  s_lastUpdateMs = now;
  
  // Read raw values
  if (s_compassType == COMPASS_HMC5883L) {
    readHMC5883L();
  } else {
    readQMC5883L();
  }
  
  if (!s_valid) return;
  
  // Update calibration if active
  if (s_calibrating) {
    if (s_rawX < s_minX) s_minX = s_rawX;
    if (s_rawX > s_maxX) s_maxX = s_rawX;
    if (s_rawY < s_minY) s_minY = s_rawY;
    if (s_rawY > s_maxY) s_maxY = s_rawY;
    if (s_rawZ < s_minZ) s_minZ = s_rawZ;
    if (s_rawZ > s_maxZ) s_maxZ = s_rawZ;
  }
  
  // Apply calibration offsets
  float x = s_rawX - ((s_maxX + s_minX) / 2.0f);
  float y = s_rawY - ((s_maxY + s_minY) / 2.0f);
  
  // Calculate heading (0-360)
  float heading = atan2(y, x) * 180.0f / M_PI;
  if (heading < 0) heading += 360.0f;
  
  // Apply low-pass filter
  float delta = heading - s_headingFiltered;
  if (delta > 180.0f) delta -= 360.0f;
  if (delta < -180.0f) delta += 360.0f;
  s_headingFiltered += delta * HEADING_FILTER_ALPHA;
  if (s_headingFiltered < 0) s_headingFiltered += 360.0f;
  if (s_headingFiltered >= 360.0f) s_headingFiltered -= 360.0f;
  
  s_headingDeg = s_headingFiltered;
}

float compassGetHeading() {
  return s_headingDeg;
}

bool compassIsValid() {
  return s_initialized && s_valid;
}

void compassGetRaw(int16_t* x, int16_t* y, int16_t* z) {
  if (x) *x = s_rawX;
  if (y) *y = s_rawY;
  if (z) *z = s_rawZ;
}

void compassStartCalibration() {
  s_calibrating = true;
  s_minX = 32767; s_maxX = -32768;
  s_minY = 32767; s_maxY = -32768;
  s_minZ = 32767; s_maxZ = -32768;
  Serial.println(F("[Compass] Calibration started - rotate WALL-E 360°"));
}

bool compassIsCalibrating() {
  return s_calibrating;
}

void compassFinishCalibration() {
  s_calibrating = false;
  Serial.printf("[Compass] Calibration complete - X:[%d,%d] Y:[%d,%d] Z:[%d,%d]\n",
    s_minX, s_maxX, s_minY, s_maxY, s_minZ, s_maxZ);
}

uint8_t compassGetType() {
  return s_compassType;
}

const char* compassGetTypeName() {
  return (s_compassType == COMPASS_HMC5883L) ? "HMC5883L" : "QMC5883L";
}
