#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ============================================================
// EVE Eyes Real Hardware Test (ESP32-S3 + WALL-E chest display)
// Single-file sketch for Arduino IDE / quick bring-up.
//
// Matches main/base chest display config:
// - Panel: ST7789, 240x240
// - SPI: MODE3
// - CS: -1 (tied low on board)
// - Pins: MOSI=11, SCK=12, DC=13, RST=14, BL=15
// ============================================================

// USER-CUSTOMIZABLE: display wiring (match your base/main chest screen)
static const int TFT_MOSI_PIN = 11;
static const int TFT_SCK_PIN  = 12;
static const int TFT_DC_PIN   = 13;
static const int TFT_RST_PIN  = 14;
static const int TFT_BL_PIN   = 15;
static const int TFT_CS_PIN   = -1;         // no CS on this panel/wiring
static const uint32_t TFT_SPI_HZ = 40000000; // 40 MHz as used in base

// USER-CUSTOMIZABLE: backlight PWM
static const uint32_t BL_PWM_FREQ = 5000;
static const uint8_t  BL_PWM_RES  = 8;      // 0..255
static const uint8_t  BL_BRIGHT   = 220;

// USER-CUSTOMIZABLE: behavior
static const uint16_t FRAME_MS = 16;        // ~60fps
static const uint16_t IDLE_GLANCE_MIN_MS = 700;
static const uint16_t IDLE_GLANCE_MAX_MS = 1800;
static const uint16_t BLINK_MIN_MS = 2000;
static const uint16_t BLINK_MAX_MS = 4600;

// Colors (565)
static const uint16_t C_BG      = ST77XX_BLACK;
static const uint16_t C_EYE     = 0xE800;   // deep red
static const uint16_t C_EYE_DIM = 0x9800;   // darker red
static const uint16_t C_PUPIL   = ST77XX_BLACK;
static const uint16_t C_HILITE  = ST77XX_WHITE;
static const uint16_t C_GRID    = 0x18E3;
static const uint16_t C_TEXT    = 0xBDF7;
static const uint16_t C_ACCENT  = 0xFBE0;

static const int W = 240;
static const int H = 240;

Adafruit_ST7789 tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

struct EyeState {
  int px = 0;
  int py = 0;
  int tx = 0;
  int ty = 0;
  float open = 1.0f; // 0..1
};

EyeState leftEye;
EyeState rightEye;

enum ExprMode : uint8_t {
  EXPR_NORMAL = 0,
  EXPR_ALERT,
  EXPR_SLEEPY,
  EXPR_CROSS
};
ExprMode expr = EXPR_NORMAL;

uint32_t nextBlinkAt = 0;
uint32_t nextGlanceAt = 0;
bool blinkClosing = false;
bool blinkOpening = false;
bool showOverlay = true;

static inline int clampi(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

static inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

static void blWrite(uint8_t level) {
  ledcWrite(TFT_BL_PIN, level);
}

static void rampBacklight(uint8_t from, uint8_t to, uint8_t step, uint16_t dly) {
  if (from < to) {
    for (uint8_t v = from; v <= to; v = (uint8_t)(v + step)) {
      blWrite(v);
      delay(dly);
      if ((uint8_t)(v + step) < v) break;
    }
  } else {
    for (int v = from; v >= to; v -= step) {
      blWrite((uint8_t)v);
      delay(dly);
    }
  }
  blWrite(to);
}

static void drawBringupGrid() {
  tft.fillScreen(C_BG);
  for (int x = 0; x < W; x += 20) tft.drawFastVLine(x, 0, H, C_GRID);
  for (int y = 0; y < H; y += 20) tft.drawFastHLine(0, y, W, C_GRID);

  tft.setTextSize(1);
  tft.setTextColor(C_TEXT, C_BG);
  tft.setCursor(8, 8);
  tft.print("ESP32-S3 ST7789 MODE3 test");
  tft.setCursor(8, 22);
  tft.print("MOSI11 SCK12 DC13 RST14 BL15");

  tft.fillRect(14, 44, 64, 24, ST77XX_RED);
  tft.fillRect(88, 44, 64, 24, ST77XX_GREEN);
  tft.fillRect(162, 44, 64, 24, ST77XX_BLUE);
  tft.drawRect(14, 44, 64, 24, ST77XX_WHITE);
  tft.drawRect(88, 44, 64, 24, ST77XX_WHITE);
  tft.drawRect(162, 44, 64, 24, ST77XX_WHITE);

  tft.setCursor(8, 80);
  tft.print("If colors are wrong/flicker:");
  tft.setCursor(8, 94);
  tft.print("- Check SPI wiring");
  tft.setCursor(8, 108);
  tft.print("- Keep MODE3");
  tft.setCursor(8, 122);
  tft.print("- Lower SPI freq to 27MHz");

  tft.setTextColor(C_ACCENT, C_BG);
  tft.setCursor(8, 206);
  tft.print("Starting eyes...");
}

static void scheduleBlink() {
  nextBlinkAt = millis() + random(BLINK_MIN_MS, BLINK_MAX_MS + 1);
}

static void scheduleGlance() {
  nextGlanceAt = millis() + random(IDLE_GLANCE_MIN_MS, IDLE_GLANCE_MAX_MS + 1);
}

static void chooseTarget() {
  const int maxX = 20;
  const int maxY = 13;
  int tx = random(-maxX, maxX + 1);
  int ty = random(-maxY, maxY + 1) - 2;
  leftEye.tx = tx;
  leftEye.ty = ty;
  rightEye.tx = tx;
  rightEye.ty = ty;

  if (expr == EXPR_CROSS) {
    leftEye.tx = clampi(tx + 7, -maxX, maxX);
    rightEye.tx = clampi(tx - 7, -maxX, maxX);
  }
}

static void updateEyes() {
  const uint32_t now = millis();
  if (now >= nextGlanceAt) {
    chooseTarget();
    scheduleGlance();
  }
  if (!blinkClosing && !blinkOpening && now >= nextBlinkAt) blinkClosing = true;

  // Smooth target interpolation
  leftEye.px += (leftEye.tx - leftEye.px) / 5;
  leftEye.py += (leftEye.ty - leftEye.py) / 5;
  rightEye.px += (rightEye.tx - rightEye.px) / 5;
  rightEye.py += (rightEye.ty - rightEye.py) / 5;

  // Blink state machine
  if (blinkClosing) {
    leftEye.open -= 0.18f;
    rightEye.open -= 0.18f;
    if (leftEye.open <= 0.04f) {
      leftEye.open = 0.04f;
      rightEye.open = 0.04f;
      blinkClosing = false;
      blinkOpening = true;
    }
  } else if (blinkOpening) {
    leftEye.open += 0.16f;
    rightEye.open += 0.16f;
    if (leftEye.open >= 1.0f) {
      leftEye.open = 1.0f;
      rightEye.open = 1.0f;
      blinkOpening = false;
      scheduleBlink();
    }
  }

  leftEye.open = clampf(leftEye.open, 0.0f, 1.0f);
  rightEye.open = clampf(rightEye.open, 0.0f, 1.0f);
}

static void drawEye(int cx, int cy, const EyeState& e, bool isLeft) {
  // Base eye shell
  tft.fillEllipse(cx, cy, 46, 34, C_EYE);
  tft.drawEllipse(cx, cy, 46, 34, C_EYE_DIM);

  // Expression tweaks
  int ox = e.px;
  int oy = e.py;
  float openness = e.open;
  if (expr == EXPR_ALERT) {
    oy -= 4;
    openness = clampf(openness + 0.12f, 0.0f, 1.0f);
  } else if (expr == EXPR_SLEEPY) {
    oy += 3;
    openness = clampf(openness - 0.32f, 0.0f, 1.0f);
  } else if (expr == EXPR_CROSS) {
    ox += isLeft ? 3 : -3;
  }

  // Pupil + highlight
  const int px = cx + ox;
  const int py = cy + oy;
  tft.fillCircle(px, py, 11, C_PUPIL);
  tft.fillCircle(px - 3, py - 3, 2, C_HILITE);

  // Eyelids from openness
  const int lid = (int)((1.0f - openness) * 36.0f);
  if (lid > 0) {
    tft.fillRect(cx - 48, cy - 36, 96, lid, C_BG);
    tft.fillRect(cx - 48, cy + 36 - lid, 96, lid, C_BG);
  }
}

static void drawEyesFrame() {
  tft.fillScreen(C_BG);
  // subtle center line
  tft.drawFastVLine(W / 2, 64, 112, 0x0841);

  const int leftCx = 74;
  const int rightCx = 166;
  const int cy = 120;
  drawEye(leftCx, cy, leftEye, true);
  drawEye(rightCx, cy, rightEye, false);

  if (showOverlay) {
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_BG);
    tft.setCursor(8, 8);
    tft.print("EVE Eye HW Test");
    tft.setCursor(8, 222);
    tft.print("n/a/s/c expr  o overlay  b blink");
  }
}

static void applySerialCommand(char c) {
  switch (c) {
    case 'n': expr = EXPR_NORMAL; break;
    case 'a': expr = EXPR_ALERT; break;
    case 's': expr = EXPR_SLEEPY; break;
    case 'c': expr = EXPR_CROSS; break;
    case 'o': showOverlay = !showOverlay; break;
    case 'b': blinkClosing = true; blinkOpening = false; break;
    default: break;
  }
}

static void pollSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    applySerialCommand(c);
  }
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println();
  Serial.println("[EVE-EYES] ESP32-S3 chest display hardware test");

  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, LOW);

  SPI.begin(TFT_SCK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN);
  SPI.setFrequency(TFT_SPI_HZ);

  pinMode(TFT_RST_PIN, OUTPUT);
  digitalWrite(TFT_RST_PIN, LOW);
  delay(200);
  digitalWrite(TFT_RST_PIN, HIGH);
  delay(200);

  tft.init(240, 240, SPI_MODE3);
  tft.setRotation(0);
  tft.fillScreen(C_BG);

  ledcAttach(TFT_BL_PIN, BL_PWM_FREQ, BL_PWM_RES);
  blWrite(8);
  delay(40);
  rampBacklight(8, BL_BRIGHT, 6, 7);

  drawBringupGrid();
  delay(1400);

  randomSeed((uint32_t)esp_random());
  chooseTarget();
  scheduleBlink();
  scheduleGlance();

  Serial.println("[EVE-EYES] Ready. Serial: n/a/s/c, o, b");
}

void loop() {
  pollSerial();
  updateEyes();
  drawEyesFrame();
  delay(FRAME_MS);
}
