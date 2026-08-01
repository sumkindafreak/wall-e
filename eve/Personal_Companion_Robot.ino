/**
 * Personal Companion Robot — RoboEyes + SSD1306 OLED **bench demo only**
 *
 * This is NOT the EVE companion firmware. EVE has no touch screen and no
 * 128x64 OLED — she uses dual LVGL eyes (see arduino_ide/EVE_Companion or
 * PlatformIO eve_s3). Use this sketch only for a separate ESP8266/ESP32 + OLED toy.
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
/** Wemos D1 Mini — optional touch pad on D5 */
#define TOUCH_PIN D5
#define I2C_SDA D2
#define I2C_SCL D1
#elif defined(ARDUINO_ARCH_ESP32)
/** ESP32 / ESP32-S3 — EVE has no touch; default -1 = eyes idle only */
#ifndef COMPANION_TOUCH_PIN
#define COMPANION_TOUCH_PIN (-1)
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
#define TOUCH_PIN (-1)
#define I2C_SDA 4
#define I2C_SCL 5
#endif

static unsigned long touchedTime = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

bool moveNE = true;

static bool touchEnabled(void) {
  return TOUCH_PIN >= 0;
}

void setup() {
  if (touchEnabled()) {
    pinMode(TOUCH_PIN, INPUT);
  }
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
  const bool touched = touchEnabled() && (digitalRead(TOUCH_PIN) == HIGH);
  unsigned long now = millis();

  if (touched) {
    roboEyes.setMood(HAPPY);
    roboEyes.setAutoblinker(OFF);
    roboEyes.setIdleMode(OFF);

    if (now - touchedTime > 150) {
      touchedTime = now;
      if (moveNE) {
        roboEyes.setPosition(NE);
      } else {
        roboEyes.setPosition(NW);
      }
      moveNE = !moveNE;
    }
  } else {
    roboEyes.setMood(DEFAULT);
    roboEyes.setAutoblinker(ON, 3, 2);
    roboEyes.setIdleMode(ON, 2, 2);
  }

  roboEyes.update();
}
