// ============================================================
//  WALL-E Master Controller — Electronic Compass Sensor
//  I2C compass for heading awareness (HMC5883L/QMC5883L compatible)
// ============================================================

#ifndef COMPASS_SENSOR_H
#define COMPASS_SENSOR_H

#include <Arduino.h>
#include <Wire.h>

// Supported compass types
#define COMPASS_HMC5883L  0
#define COMPASS_QMC5883L  1

// I2C addresses
#define COMPASS_ADDR_HMC  0x1E
#define COMPASS_ADDR_QMC  0x0D

// Update rate
#define COMPASS_UPDATE_MS  100   // 10Hz update

// Calibration
#define COMPASS_CALIB_SAMPLES  50

// Initialize compass (auto-detect type)
bool compassInit();

// Update compass (call every loop)
void compassUpdate(uint32_t now);

// Get current heading (0-360 degrees, 0=North)
float compassGetHeading();

// Check if compass is valid
bool compassIsValid();

// Get raw magnetometer values
void compassGetRaw(int16_t* x, int16_t* y, int16_t* z);

// Calibrate compass (collect min/max over time)
void compassStartCalibration();
bool compassIsCalibrating();
void compassFinishCalibration();

// Get compass type
uint8_t compassGetType();
const char* compassGetTypeName();

// Orientation control (for autonomy)
void compassSetTargetHeading(float degrees);  // 0-360
float compassGetTargetHeading();
float compassGetHeadingError();  // Signed error in degrees (-180 to +180)

#endif // COMPASS_SENSOR_H
