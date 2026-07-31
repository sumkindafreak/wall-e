/**
 * Ghostbusters Slime Blower — firmware v2.0
 *
 * ESP32-S3 DevKitC-1 + FastLED + DFPlayer Mini.
 * Drive hardware: 4× N-channel MOSFETs (low-side or per your board) — no L298,
 * no relay modules. Blower motor = PWM on one gate; three digital outputs for
 * former relay loads (fans, valves, etc. — wire as needed).
 *
 * DFPlayer: UART (RX/TX) + power only — BUSY pin not used.
 *
 * SD: 001.mp3 startup, 002.mp3 slime blow (loop while trigger held).
 */

#include <Arduino.h>
#include "showduino_version.h"
#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>

// ---------------------------------------------------------------------------
// MOSFET outputs (replaces L298 motor speed + relay coils)
// ---------------------------------------------------------------------------
static const int PIN_MOSFET_BLOWER = 11;  // PWM — blower motor (was L298 / enable)
static const int PIN_MOSFET_1      = 12;  // digital — ex relay channel 1
static const int PIN_MOSFET_2      = 13;  // digital — ex relay channel 2
static const int PIN_MOSFET_3      = 14;  // digital — ex relay channel 3

#define BLOWER_PWM_FREQ   20000
#define BLOWER_PWM_BITS   8
/** PWM duty 0–255 when blowing (full speed). Lower for a weaker blow if desired. */
static const uint8_t BLOWER_SPEED_ON = 255;

// ---------------------------------------------------------------------------
// Pins — LEDs, trigger, DFPlayer, analog
// ---------------------------------------------------------------------------
static const int PIN_LED_DATA = 8;   // WS2812 data
static const int PIN_TRIGGER  = 9;   // active LOW, blower trigger
static const int PIN_DF_TX    = 17;  // ESP TX -> DFPlayer RX
static const int PIN_DF_RX    = 18;  // ESP RX <- DFPlayer TX
static const int PIN_VOL_ADC  = 7;   // analog master volume (ADC1)
static const int PIN_TANK_ADC = 6;   // analog tank / charge level

// ---------------------------------------------------------------------------
// LEDs
// ---------------------------------------------------------------------------
static const int NUM_LEDS = 24;
static const uint8_t LED_BRIGHTNESS = 200;

CRGB leds[NUM_LEDS];

// ---------------------------------------------------------------------------
// DFPlayer
// ---------------------------------------------------------------------------
#define DFPLAYER_BAUD 9600

HardwareSerial DFSerial(1);
DFRobotDFPlayerMini df;

static const uint8_t TRACK_STARTUP = 1;
static const uint8_t TRACK_BLOW   = 2;

bool g_dfOk = false;

void stopTrack() {
  /* no-op: df.stop() caused lockups on some modules */
}

static void mosfetsInit() {
  ledcAttach(PIN_MOSFET_BLOWER, BLOWER_PWM_FREQ, BLOWER_PWM_BITS);
  pinMode(PIN_MOSFET_1, OUTPUT);
  pinMode(PIN_MOSFET_2, OUTPUT);
  pinMode(PIN_MOSFET_3, OUTPUT);
  ledcWrite(PIN_MOSFET_BLOWER, 0);
  digitalWrite(PIN_MOSFET_1, LOW);
  digitalWrite(PIN_MOSFET_2, LOW);
  digitalWrite(PIN_MOSFET_3, LOW);
}

/** BLOWER = PWM; three aux MOSFETs = on/off with blow (ex-relay timing). */
static void setMosfetOutputs(bool blowing) {
  if (blowing) {
    ledcWrite(PIN_MOSFET_BLOWER, BLOWER_SPEED_ON);
    digitalWrite(PIN_MOSFET_1, HIGH);
    digitalWrite(PIN_MOSFET_2, HIGH);
    digitalWrite(PIN_MOSFET_3, HIGH);
  } else {
    ledcWrite(PIN_MOSFET_BLOWER, 0);
    digitalWrite(PIN_MOSFET_1, LOW);
    digitalWrite(PIN_MOSFET_2, LOW);
    digitalWrite(PIN_MOSFET_3, LOW);
  }
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static unsigned long s_lastVolMs = 0;
static uint8_t s_lastVolCmd = 255;
static bool s_wasBlowing = false;

static int readTankPercent() {
  int raw = analogRead(PIN_TANK_ADC);
  return constrain(map(raw, 0, 4095, 0, 100), 0, 100);
}

static void applyVolumeFromKnob() {
  if (!g_dfOk) return;
  if (millis() - s_lastVolMs < 40) return;
  s_lastVolMs = millis();

  int raw = analogRead(PIN_VOL_ADC);
  uint8_t v = (uint8_t)map(raw, 0, 4095, 0, 30);
  if (v != s_lastVolCmd) {
    s_lastVolCmd = v;
    df.volume(v);
  }
}

static bool triggerDown() {
  return digitalRead(PIN_TRIGGER) == LOW;
}

static void updateLights(int tankPct, bool blowing, bool criticalLow) {
  static unsigned long blinkMs = 0;
  static bool blinkOn = false;

  if (criticalLow) {
    if (millis() - blinkMs > 120) {
      blinkMs = millis();
      blinkOn = !blinkOn;
    }
    const CRGB c = blinkOn ? CRGB::Red : CRGB::Black;
    fill_solid(leds, NUM_LEDS, c);
    FastLED.show();
    return;
  }

  int litCount = map(tankPct, 0, 100, 0, NUM_LEDS);
  litCount = constrain(litCount, 0, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < litCount) {
      uint8_t hue = (uint8_t)map(i, 0, max(1, NUM_LEDS - 1), 96, 0);
      leds[i] = CHSV(hue, 255, 255);
    } else {
      leds[i] = CRGB::Black;
    }
  }

  if (blowing) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].g = min(255, (int)leds[i].g + 60);
      leds[i].b = min(255, (int)leds[i].b + 20);
    }
  }

  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  delay(80);
  SHOWDUINO_LOG_BOOT_VERSION("ghostbusters");
  Serial.println(F("\nSlime Blower v2.0"));

  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  mosfetsInit();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  DFSerial.begin(DFPLAYER_BAUD, SERIAL_8N1, PIN_DF_RX, PIN_DF_TX);
  delay(500);

  if (df.begin(DFSerial, true, true)) {
    g_dfOk = true;
    df.EQ(DFPLAYER_EQ_NORMAL);
    df.volume(20);
    s_lastVolCmd = 20;
    df.play(TRACK_STARTUP);
    Serial.println(F("DFPlayer OK"));
  } else {
    Serial.println(F("DFPlayer init failed — LEDs + MOSFETs only"));
  }
}

void loop() {
  applyVolumeFromKnob();

  int tank = readTankPercent();
  const bool critical = (tank < 10);
  const bool canBlow = (tank > 5);
  const bool blowing = triggerDown() && canBlow && !critical;

  setMosfetOutputs(blowing);
  updateLights(tank, blowing, critical);

  if (g_dfOk) {
    if (blowing && !s_wasBlowing) {
      df.loop(TRACK_BLOW);
    }
    if (!blowing && s_wasBlowing) {
      stopTrack();
    }
    s_wasBlowing = blowing;
  }

  delay(4);
}
