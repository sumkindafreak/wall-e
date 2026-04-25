#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ============================================================
// EVE Eyes Test (single-file Wokwi sketch)
// Board: ESP32
// Display: ILI9341 SPI TFT
//
// Default Wokwi pins (edit if needed):
// TFT_CS   -> 15
// TFT_DC   -> 2
// TFT_RST  -> 4
// TFT_SCK  -> 18
// TFT_MOSI -> 23
// TFT_MISO -> 19
// ============================================================

// USER-CUSTOMIZABLE: TFT pin mapping
static const int TFT_CS_PIN  = 15;
static const int TFT_DC_PIN  = 2;
static const int TFT_RST_PIN = 4;

Adafruit_ILI9341 tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);

// USER-CUSTOMIZABLE: eye style/colors
static const uint16_t C_BG       = ILI9341_BLACK;
static const uint16_t C_EYE      = ILI9341_RED;
static const uint16_t C_PUPIL    = ILI9341_BLACK;
static const uint16_t C_HILITE   = ILI9341_WHITE;
static const uint16_t C_EYELID   = ILI9341_BLACK;

// USER-CUSTOMIZABLE: geometry
static const int SCREEN_W = 320;
static const int SCREEN_H = 240;
static const int EYE_RX   = 56;  // eye width radius
static const int EYE_RY   = 42;  // eye height radius
static const int EYE_GAP  = 36;  // center gap between eyes
static const int PUPIL_R  = 14;

static const int LEFT_CX  = (SCREEN_W / 2) - (EYE_RX + (EYE_GAP / 2));
static const int RIGHT_CX = (SCREEN_W / 2) + (EYE_RX + (EYE_GAP / 2));
static const int EYE_CY   = (SCREEN_H / 2);

// USER-CUSTOMIZABLE: motion/blink timing
static const uint32_t GLANCE_MIN_MS = 650;
static const uint32_t GLANCE_MAX_MS = 1700;
static const uint32_t BLINK_MIN_MS  = 1800;
static const uint32_t BLINK_MAX_MS  = 5000;
static const uint16_t FRAME_MS      = 16; // ~60 FPS

struct EyeState {
  int pupilX = 0;       // offset from center
  int pupilY = 0;       // offset from center
  int targetX = 0;
  int targetY = 0;
  float openness = 1.0; // 0..1 (blink)
};

EyeState leftEye;
EyeState rightEye;

uint32_t nextGlanceAt = 0;
uint32_t nextBlinkAt = 0;
bool blinkClosing = false;
bool blinkOpening = false;

// Optional expression mode via Serial:
// n = normal, a = alert (wide), s = sleepy (droopy), c = curious (cross-eye)
char expressionMode = 'n';

static int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static float clampFloat(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void scheduleNextGlance() {
  nextGlanceAt = millis() + random(GLANCE_MIN_MS, GLANCE_MAX_MS + 1);
}

static void scheduleNextBlink() {
  nextBlinkAt = millis() + random(BLINK_MIN_MS, BLINK_MAX_MS + 1);
}

static void pickNewGazeTarget() {
  int maxX = EYE_RX - PUPIL_R - 8;
  int maxY = EYE_RY - PUPIL_R - 8;
  int tx = random(-maxX, maxX + 1);
  int ty = random(-maxY, maxY + 1);

  // Slightly bias Y upward for a more "alive" look.
  ty -= 3;

  leftEye.targetX = tx;
  leftEye.targetY = ty;
  rightEye.targetX = tx;
  rightEye.targetY = ty;

  if (expressionMode == 'c') {
    leftEye.targetX = clampInt(tx + 10, -maxX, maxX);
    rightEye.targetX = clampInt(tx - 10, -maxX, maxX);
  }
}

static void updateSerialExpression() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == 'n' || c == 'a' || c == 's' || c == 'c') {
      expressionMode = c;
      Serial.print("Expression mode: ");
      Serial.println(expressionMode);
    }
  }
}

static void updateBlink() {
  const uint32_t now = millis();
  if (!blinkClosing && !blinkOpening && now >= nextBlinkAt) {
    blinkClosing = true;
  }

  if (blinkClosing) {
    leftEye.openness -= 0.16f;
    rightEye.openness -= 0.16f;
    if (leftEye.openness <= 0.03f) {
      leftEye.openness = 0.03f;
      rightEye.openness = 0.03f;
      blinkClosing = false;
      blinkOpening = true;
    }
  } else if (blinkOpening) {
    leftEye.openness += 0.15f;
    rightEye.openness += 0.15f;
    if (leftEye.openness >= 1.0f) {
      leftEye.openness = 1.0f;
      rightEye.openness = 1.0f;
      blinkOpening = false;
      scheduleNextBlink();
    }
  }

  leftEye.openness = clampFloat(leftEye.openness, 0.0f, 1.0f);
  rightEye.openness = clampFloat(rightEye.openness, 0.0f, 1.0f);
}

static void updateGaze() {
  const uint32_t now = millis();
  if (now >= nextGlanceAt) {
    pickNewGazeTarget();
    scheduleNextGlance();
  }

  // Smooth interpolation toward target.
  leftEye.pupilX += (leftEye.targetX - leftEye.pupilX) / 5;
  leftEye.pupilY += (leftEye.targetY - leftEye.pupilY) / 5;
  rightEye.pupilX += (rightEye.targetX - rightEye.pupilX) / 5;
  rightEye.pupilY += (rightEye.targetY - rightEye.pupilY) / 5;
}

static void drawEye(int cx, int cy, const EyeState &e, bool isLeft) {
  // Base eye
  tft.fillEllipse(cx, cy, EYE_RX, EYE_RY, C_EYE);

  // Pupil position with expression offsets
  int px = cx + e.pupilX;
  int py = cy + e.pupilY;

  if (expressionMode == 'a') {
    py -= 4; // alert: eyes looking slightly up
  } else if (expressionMode == 's') {
    py += 3; // sleepy: slightly down
  } else if (expressionMode == 'c') {
    px += isLeft ? 4 : -4; // curious: tiny inward pull
  }

  tft.fillCircle(px, py, PUPIL_R, C_PUPIL);
  tft.fillCircle(px - 4, py - 4, 3, C_HILITE);

  // Blink/eyelid mask (top and bottom)
  float open = e.openness;
  if (expressionMode == 'a') open = clampFloat(open + 0.12f, 0.0f, 1.0f);
  if (expressionMode == 's') open = clampFloat(open - 0.28f, 0.0f, 1.0f);

  int lid = (int)((1.0f - open) * (EYE_RY + 2));
  if (lid > 0) {
    // Upper lid
    tft.fillRect(cx - EYE_RX - 2, cy - EYE_RY - 2, (EYE_RX * 2) + 4, lid + 2, C_EYELID);
    // Lower lid
    tft.fillRect(cx - EYE_RX - 2, cy + EYE_RY - lid, (EYE_RX * 2) + 4, lid + 2, C_EYELID);
  }
}

static void drawFrame() {
  tft.fillScreen(C_BG);

  // Subtle center bridge/shadow
  tft.drawFastVLine(SCREEN_W / 2, EYE_CY - 42, 84, ILI9341_DARKGREY);

  drawEye(LEFT_CX, EYE_CY, leftEye, true);
  drawEye(RIGHT_CX, EYE_CY, rightEye, false);
}

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());

  tft.begin();
  tft.setRotation(1); // landscape
  tft.fillScreen(C_BG);

  pickNewGazeTarget();
  scheduleNextGlance();
  scheduleNextBlink();

  Serial.println("EVE Eyes Test ready.");
  Serial.println("Serial commands: n=normal, a=alert, s=sleepy, c=curious");
}

void loop() {
  updateSerialExpression();
  updateBlink();
  updateGaze();
  drawFrame();
  delay(FRAME_MS);
}
