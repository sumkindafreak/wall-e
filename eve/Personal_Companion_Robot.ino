#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define TOUCH D5

static unsigned long touchedTime = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

bool moveNE = true;

void setup() {
  pinMode(TOUCH, INPUT);
  Wire.begin(D2, D1);

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
  bool touched = digitalRead(TOUCH);
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
