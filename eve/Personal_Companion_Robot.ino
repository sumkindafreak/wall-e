/**
 * Personal Companion Robot — RoboEyes + SSD1306 OLED demo
 *
 * Originally wired for ESP8266 NodeMCU (D1/D2/D5 names).
 * Also builds on ESP32 / ESP32-S3 when you pick the correct board in Arduino IDE.
 *
 * Libraries: Adafruit GFX, Adafruit SSD1306, FluxGarage RoboEyes
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#if defined(ARDUINO_ARCH_ESP8266)
/** Wemos D1 Mini / NodeMCU style */
#define TOUCH_PIN D5
#define I2C_SDA D2
#define I2C_SCL D1
#elif defined(ARDUINO_ARCH_ESP32)
/** ESP32 / ESP32-S3 — match your OLED + touch wiring */
#ifndef COMPANION_TOUCH_PIN
#define COMPANION_TOUCH_PIN 14
#endif
#ifndef COMPANION_I2C_SDA
#define COMPANION_I2C_SDA 8
#endif
#ifndef COMPANION_I2C_SCL
#define COMPANION_I2C_SCL 9
#endif
#define TOUCH_PIN COMPANION_TOUCH_PIN
#define I2C_SDA COMPANION_I2C_SDA
#define I2C_SCL COMPANION_I2C_SCL
#else
/** Generic fallback (NodeMCU GPIO numbers) */
#define TOUCH_PIN 14
#define I2C_SDA 4
#define I2C_SCL 5
#endif

static unsigned long touchedTime = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

bool moveNE = true;

void setup() {
  pinMode(TOUCH_PIN, INPUT);
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setPosition(DEFAULT);
  roboEyes.setMood(DEFAULT);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);

  touchedTime = millis();
}

void loop() {
  bool touched = digitalRead(TOUCH_PIN);
  unsigned long now = millis();

  // ========= TAP / TOUCH =========
  if (touched == HIGH) {

    roboEyes.setMood(HAPPY);
    roboEyes.setAutoblinker(OFF);
    roboEyes.setIdleMode(OFF);

    // smooth NE <-> NW movement
    if (now - touchedTime > 150) {
      touchedTime = now;

      if (moveNE) {
        roboEyes.setPosition(NE);
      } else {
        roboEyes.setPosition(NW);
      }
      moveNE = !moveNE;
    }
  }

  // ========= NORMAL IDLE =========
  else {
    roboEyes.setMood(DEFAULT);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
    // DO NOT force setPosition(DEFAULT)
  }

  roboEyes.update();
}
