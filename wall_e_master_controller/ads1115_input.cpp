// ==========================================================
// ADS1115 Implementation
// ==========================================================

#include "ads1115_input.h"
#include "i2c_devices.h"
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// Calibration
static int16_t centerRaw[4] = {0, 0, 0, 0};
static const int16_t ADC_RANGE = 17000;
static bool calibrated = false;

// Tuning - MAXIMUM SPEED configuration
static float deadzone = 0.03f;   // 3% deadzone (minimal)
static float expo = 0.0f;        // 0% expo = LINEAR response (fastest!)
static float maxOutput = 1.0f;   // 100% max speed (full power)

// State
static JoystickState joyState = {};

#define CAL_SAMPLES 100
#define CAL_INTERVAL_MS 5

bool ads1115Init() {
  Serial.println(F("[ADS1115] Initializing..."));
  
  if (!ads.begin(ADS1115_ADDR)) {
    Serial.println(F("[ADS1115] ❌ Not found"));
    return false;
  }
  
  ads.setGain(GAIN_TWOTHIRDS);
  ads.setDataRate(RATE_ADS1115_860SPS);
  
  Serial.println(F("[ADS1115] ✓ Found"));
  Serial.println(F("[ADS1115] Calibrating (keep sticks centered)..."));
  
  long sum[4] = {0, 0, 0, 0};
  for (int i = 0; i < CAL_SAMPLES; i++) {
    for (int ch = 0; ch < 4; ch++) {
      sum[ch] += ads.readADC_SingleEnded(ch);
    }
    delay(CAL_INTERVAL_MS);
  }
  
  for (int ch = 0; ch < 4; ch++) {
    centerRaw[ch] = sum[ch] / CAL_SAMPLES;
  }
  
  calibrated = true;
  
  Serial.println(F("[ADS1115] ✓ Calibration complete"));
  Serial.printf("  Centers: [%d, %d, %d, %d]\n", 
    centerRaw[0], centerRaw[1], centerRaw[2], centerRaw[3]);
  
  return true;
}

static float processAxis(float raw) {
  if (fabsf(raw) < deadzone) return 0.0f;
  
  float sign = (raw > 0) ? 1.0f : -1.0f;
  float abs_val = fabsf(raw);
  float scaled = (abs_val - deadzone) / (1.0f - deadzone);
  scaled = constrain(scaled, 0.0f, 1.0f);
  
  float curved = expo * scaled * scaled * scaled + (1.0f - expo) * scaled;
  curved *= maxOutput;
  
  return sign * curved;
}

void ads1115Update() {
  if (!calibrated) return; // Skip if not initialized
  
  static int channelIndex = 0;
  
  int16_t rawAdc = ads.readADC_SingleEnded(channelIndex);
  int16_t delta = rawAdc - centerRaw[channelIndex];
  float normalized = constrain((float)delta / (float)ADC_RANGE, -1.0f, 1.0f);
  
  joyState.raw[channelIndex] = normalized;
  joyState.processed[channelIndex] = processAxis(normalized);
  
  // Detect activity change and log it
  bool wasActive = joyState.active[channelIndex];
  joyState.active[channelIndex] = (fabsf(joyState.processed[channelIndex]) > 0.001f);
  
  // Log when joystick starts/stops moving
  if (joyState.active[channelIndex] && !wasActive) {
    const char* axisNames[] = {"Joy1_X", "Joy1_Y", "Joy2_X", "Joy2_Y"};
    Serial.printf("[Joystick] %s activated: %.2f\n", axisNames[channelIndex], joyState.processed[channelIndex]);
  }
  
  // Log continuous movement every 20 loops (once per full scan cycle + some)
  static int loopCounter = 0;
  loopCounter++;
  if (loopCounter >= 80 && joyState.active[channelIndex]) {  // ~20 full cycles
    const char* axisNames[] = {"Joy1_X", "Joy1_Y", "Joy2_X", "Joy2_Y"};
    Serial.printf("[Joystick] %s: raw=%.2f, proc=%.2f\n", 
      axisNames[channelIndex], joyState.raw[channelIndex], joyState.processed[channelIndex]);
    loopCounter = 0;
  }
  
  channelIndex = (channelIndex + 1) % 4;
}

const JoystickState& getJoystickState() {
  return joyState;
}

void joystickToDriveState(DriveState* ds) {
  // Single stick tank drive (Joy2)
  // Y-axis = forward/back throttle
  // X-axis = left/right turn
  
  float throttle = -joyState.processed[JOY2_Y];  // INVERTED: forward is positive
  float turn = joyState.processed[JOY2_X];       // Right is positive
  
  // Tank mixing: differential steering
  float left = throttle + turn;
  float right = throttle - turn;
  
  // Constrain to valid range
  left = constrain(left, -1.0f, 1.0f);
  right = constrain(right, -1.0f, 1.0f);
  
  ds->leftSpeed = (int8_t)(left * 100.0f);
  ds->rightSpeed = (int8_t)(right * 100.0f);
  ds->precisionMode = false;
  
  // Debug output when values change significantly
  static int8_t lastLeft = 0, lastRight = 0;
  if (abs(ds->leftSpeed - lastLeft) > 5 || abs(ds->rightSpeed - lastRight) > 5) {
    Serial.printf("[Tank] Throttle=%.2f Turn=%.2f → L=%d%% R=%d%%\n", 
      throttle, turn, ds->leftSpeed, ds->rightSpeed);
    lastLeft = ds->leftSpeed;
    lastRight = ds->rightSpeed;
  }
}

void setJoystickDeadzone(float dz) { deadzone = constrain(dz, 0.0f, 0.5f); }
void setJoystickExpo(float exp_val) { expo = constrain(exp_val, 0.0f, 1.0f); }
void setJoystickMaxOutput(float max) { maxOutput = constrain(max, 0.0f, 1.0f); }
