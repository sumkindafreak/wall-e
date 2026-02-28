// ============================================================
//  WALL-E Servo Manager Implementation
//  Smooth velocity-controlled servo movements via PCA9685
// ============================================================

#include "servo_manager.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Arduino.h>

// ============================================================
//  PCA9685 instance
// ============================================================
static Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(PCA9685_ADDR);

// ============================================================
//  Calibration table — LOW/HIGH pulse widths per channel
//  Maps 0-100 position range to actual microsecond pulse
// ============================================================
static const int CAL_LO[SERVO_COUNT] = {
  CAL_HEAD_PAN_LO,
  CAL_NECK_TOP_LO,
  CAL_NECK_BOT_LO,
  CAL_EYE_RIGHT_LO,
  CAL_EYE_LEFT_LO,
  CAL_ARM_LEFT_LO,
  CAL_ARM_RIGHT_LO,
  CAL_BROW_LEFT_LO,
  CAL_BROW_RIGHT_LO
};

static const int CAL_HI[SERVO_COUNT] = {
  CAL_HEAD_PAN_HI,
  CAL_NECK_TOP_HI,
  CAL_NECK_BOT_HI,
  CAL_EYE_RIGHT_HI,
  CAL_EYE_LEFT_HI,
  CAL_ARM_LEFT_HI,
  CAL_ARM_RIGHT_HI,
  CAL_BROW_LEFT_HI,
  CAL_BROW_RIGHT_HI
};

static const int NEUTRAL_POS[SERVO_COUNT] = {
  NEUTRAL_HEAD_PAN,
  NEUTRAL_NECK_TOP,
  NEUTRAL_NECK_BOT,
  NEUTRAL_EYE_RIGHT,
  NEUTRAL_EYE_LEFT,
  NEUTRAL_ARM_LEFT,
  NEUTRAL_ARM_RIGHT,
  NEUTRAL_BROW_LEFT,
  NEUTRAL_BROW_RIGHT
};

// ============================================================
//  Servo state — float for smooth sub-step interpolation
// ============================================================
struct ServoState {
  float  current;    // current position 0.0-100.0
  float  target;     // target position 0.0-100.0
  float  speed;      // units per millisecond
  bool   enabled;
};

static ServoState servo[SERVO_COUNT];
static unsigned long _lastUpdate = 0;
#define UPDATE_INTERVAL_MS  20   // 50Hz servo update rate

// ============================================================
//  Internal: convert 0-100 position to microseconds
// ============================================================
static int posToMicros(uint8_t ch, float pos) {
  pos = constrain(pos, 0.0f, 100.0f);
  return (int)map((long)(pos * 10), 0, 1000, CAL_LO[ch], CAL_HI[ch]);
}

// ============================================================
//  Internal: write microseconds to PCA9685
// ============================================================
static void writeMicros(uint8_t ch, int us) {
  // PCA9685 at 50Hz: period = 20000us, 4096 ticks
  // tick = us * 4096 / 20000
  uint16_t pulse = (uint16_t)((us * 4096UL) / 20000UL);
  pca.setPWM(ch, 0, pulse);
}

// ============================================================
//  Public API
// ============================================================

void servoInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  pca.begin();
  pca.setOscillatorFrequency(PCA_OSC_FREQ);
  pca.setPWMFreq(50);   // standard servo frequency
  delay(10);

  // Initialise state and move to neutral slowly
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    servo[i].current = (float)NEUTRAL_POS[i];
    servo[i].target  = (float)NEUTRAL_POS[i];
    servo[i].speed   = SERVO_SLOW_SPEED / 1000.0f;  // units per ms
    servo[i].enabled = true;
    writeMicros(i, posToMicros(i, servo[i].current));
  }

  _lastUpdate = millis();
  Serial.printf("[Servos] PCA9685 initialised at 0x%02X, %d channels\n", PCA9685_ADDR, SERVO_COUNT);
}

void servoHandle() {
  unsigned long now = millis();
  if ((now - _lastUpdate) < UPDATE_INTERVAL_MS) return;
  float dt = (float)(now - _lastUpdate);
  _lastUpdate = now;

  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (!servo[i].enabled) continue;
    if (servo[i].current == servo[i].target) continue;

    float diff = servo[i].target - servo[i].current;
    float step = servo[i].speed * dt;

    if (fabsf(diff) <= step) {
      servo[i].current = servo[i].target;
    } else {
      servo[i].current += (diff > 0) ? step : -step;
    }
    writeMicros(i, posToMicros(i, servo[i].current));
  }
}

void servoSet(uint8_t ch, int pos, int speed) {
  if (ch >= SERVO_COUNT) return;
  servo[ch].target = constrain((float)pos, 0.0f, 100.0f);
  // speed 0-100 maps to 0.02-0.2 units/ms (tunable)
  float spd = constrain(speed, 1, 100);
  servo[ch].speed = 0.02f + (spd / 100.0f) * 0.18f;
}

void servoSetAll(int positions[SERVO_COUNT], int speed) {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (positions[i] >= 0) {   // -1 means skip this servo
      servoSet(i, positions[i], speed);
    }
  }
}

void servoNeutral(int speed) {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    servoSet(i, NEUTRAL_POS[i], speed);
  }
}

int servoGetPos(uint8_t ch) {
  if (ch >= SERVO_COUNT) return 50;
  return (int)servo[ch].target;
}

bool servoIsMoving() {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (fabsf(servo[i].current - servo[i].target) > 0.5f) return true;
  }
  return false;
}

String servoGetStatusJSON() {
  String s = "{\"servos\":[";
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (i > 0) s += ",";
    s += "{\"ch\":" + String(i);
    s += ",\"pos\":" + String((int)servo[i].target);
    s += ",\"cur\":" + String((int)servo[i].current);
    s += "}";
  }
  s += "]}";
  return s;
}
