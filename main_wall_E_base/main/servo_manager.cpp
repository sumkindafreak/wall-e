// ============================================================
//  WALL-E Servo Manager — PCA9685 + acceleration-limited smoothing
//  (reference-style: velocity + accel per channel, dt from micros)
// ============================================================

#include "servo_manager.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Arduino.h>
#include <math.h>

static Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(PCA9685_ADDR);

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

// Max velocity (%/s) and acceleration (%/s²) — tuned from reference proportions
static const float SERVO_MAXVEL[SERVO_COUNT] = {
  130.f, 110.f, 130.f,
  380.f, 380.f,
  160.f, 160.f,
  220.f, 220.f
};
static const float SERVO_ACCEL[SERVO_COUNT] = {
  180.f, 150.f, 180.f,
  2200.f, 2200.f,
  220.f, 220.f,
  350.f, 350.f
};

#define SERVO_UPDATE_MS       10
#define SERVO_POS_THRESHOLD   0.12f

struct ServoState {
  float current;
  float target;
  float curvel;    // % per second
  float vel_scale; // scales maxvel/accel from servoSet(..., speed)
  bool  enabled;
};

static ServoState servo[SERVO_COUNT];
static unsigned long lastServoUpdateMs = 0;
static unsigned long lastServoMicros = 0;

static int posToMicros(uint8_t ch, float pos) {
  pos = constrain(pos, 0.0f, 100.0f);
  return (int)map((long)(pos * 10), 0, 1000, CAL_LO[ch], CAL_HI[ch]);
}

static void writeMicros(uint8_t ch, int us) {
  uint16_t pulse = (uint16_t)((us * 4096UL) / 20000UL);
  pca.setPWM(ch, 0, pulse);
}

void servoInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  pca.begin();
  pca.setOscillatorFrequency(PCA_OSC_FREQ);
  pca.setPWMFreq(50);
  delay(10);

  unsigned long now = millis();
  lastServoUpdateMs = now;
  lastServoMicros = micros();

  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    float p = (float)NEUTRAL_POS[i];
    servo[i].current = p;
    servo[i].target = p;
    servo[i].curvel = 0.f;
    servo[i].vel_scale = 1.0f;
    servo[i].enabled = true;
    writeMicros(i, posToMicros(i, p));
  }

  Serial.printf("[Servos] PCA9685 initialised at 0x%02X, %d channels (accel smoothing)\n",
                PCA9685_ADDR, SERVO_COUNT);
}

void servoHandle() {
  unsigned long nowMs = millis();
  if ((nowMs - lastServoUpdateMs) < SERVO_UPDATE_MS) return;
  lastServoUpdateMs = nowMs;

  unsigned long nowMicros = micros();
  float dtMs = (nowMicros - lastServoMicros) / 1000.0f;
  lastServoMicros = nowMicros;
  if (dtMs < 0.5f || dtMs > 40.0f) dtMs = (float)SERVO_UPDATE_MS;

  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (!servo[i].enabled) continue;

    float posError = servo[i].target - servo[i].current;
    float maxv = SERVO_MAXVEL[i] * servo[i].vel_scale;
    float acc = SERVO_ACCEL[i] * servo[i].vel_scale;

    if (fabsf(posError) > SERVO_POS_THRESHOLD) {
      float acceleration = acc;
      if (acc > 0.0001f &&
          (servo[i].curvel * servo[i].curvel / (2.0f * acc)) > fabsf(posError)) {
        acceleration = -acc;
      }

      if (posError > 0.0f) {
        servo[i].curvel += acceleration * dtMs / 1000.0f;
      } else {
        servo[i].curvel -= acceleration * dtMs / 1000.0f;
      }

      if (servo[i].curvel > maxv) servo[i].curvel = maxv;
      if (servo[i].curvel < -maxv) servo[i].curvel = -maxv;

      float dP = servo[i].curvel * dtMs / 1000.0f;
      if (fabsf(dP) < fabsf(posError)) {
        servo[i].current += dP;
      } else {
        servo[i].current = servo[i].target;
        servo[i].curvel = 0.f;
      }
    } else {
      servo[i].curvel = 0.f;
      servo[i].current = servo[i].target;
    }

    writeMicros(i, posToMicros(i, servo[i].current));
  }
}

void servoSet(uint8_t ch, int pos, int speed) {
  if (ch >= SERVO_COUNT) return;
  servo[ch].target = constrain((float)pos, 0.0f, 100.0f);
  int spd = constrain(speed, 1, 100);
  servo[ch].vel_scale = 0.25f + 0.75f * (spd / 100.0f);
}

void servoSetAll(int positions[SERVO_COUNT], int speed) {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (positions[i] >= 0) {
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
    if (fabsf(servo[i].current - servo[i].target) > SERVO_POS_THRESHOLD) return true;
    if (fabsf(servo[i].curvel) > 0.5f) return true;
  }
  return false;
}

String servoGetStatusJSON() {
  String s = "{\"servos\":[";
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (i > 0) s += ",";
    s += "{\"ch\":" + String(i);
    s += ",\"pos\":" + String((int)servo[i].target);
    s += ",\"cur\":" + String((int)(servo[i].current + 0.5f));
    s += "}";
  }
  s += "]}";
  return s;
}
