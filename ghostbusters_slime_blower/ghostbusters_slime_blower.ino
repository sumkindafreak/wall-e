/**
 * Ghostbusters Slime Blower — firmware v2.1
 *
 * ESP32-S3 DevKitC-1 + FastLED + SD-card WAV over I2S.
 * Drive hardware: 4× N-channel MOSFETs (low-side or per your board).
 *
 * SD: /audio/001.wav startup, /audio/002.wav slime blow (loop while trigger held).
 */

#include <Arduino.h>
#include <FastLED.h>
#include <SD.h>
#include <SPI.h>
#include "walle_i2s_wav_player.h"

static const int PIN_MOSFET_BLOWER = 11;
static const int PIN_MOSFET_1 = 12;
static const int PIN_MOSFET_2 = 13;
static const int PIN_MOSFET_3 = 14;

#define BLOWER_PWM_FREQ 20000
#define BLOWER_PWM_BITS 8
static const uint8_t BLOWER_SPEED_ON = 255;

static const int PIN_LED_DATA = 8;
static const int PIN_TRIGGER = 9;
static const int PIN_SD_CS = 1;
static const int PIN_SD_MOSI = 2;
static const int PIN_SD_MISO = 3;
static const int PIN_SD_SCK = 4;
static const int PIN_I2S_BCLK = 15;
static const int PIN_I2S_LRCK = 16;
static const int PIN_I2S_DOUT = 21;
static const int PIN_VOL_ADC = 7;
static const int PIN_TANK_ADC = 6;

static const int NUM_LEDS = 24;
static const uint8_t LED_BRIGHTNESS = 200;

CRGB leds[NUM_LEDS];

static bool g_audioOk = false;
static bool g_sdOk = false;

static void stopTrack(void) {
  walleI2sAudioStop();
}

static void mosfetsInit(void) {
  ledcAttach(PIN_MOSFET_BLOWER, BLOWER_PWM_FREQ, BLOWER_PWM_BITS);
  pinMode(PIN_MOSFET_1, OUTPUT);
  pinMode(PIN_MOSFET_2, OUTPUT);
  pinMode(PIN_MOSFET_3, OUTPUT);
  ledcWrite(PIN_MOSFET_BLOWER, 0);
  digitalWrite(PIN_MOSFET_1, LOW);
  digitalWrite(PIN_MOSFET_2, LOW);
  digitalWrite(PIN_MOSFET_3, LOW);
}

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

static unsigned long s_lastVolMs = 0;
static uint8_t s_lastVolCmd = 255;
static bool s_wasBlowing = false;

static int readTankPercent(void) {
  int raw = analogRead(PIN_TANK_ADC);
  return constrain(map(raw, 0, 4095, 0, 100), 0, 100);
}

static void applyVolumeFromKnob(void) {
  if (!g_audioOk) {
    return;
  }
  if (millis() - s_lastVolMs < 40) {
    return;
  }
  s_lastVolMs = millis();

  int raw = analogRead(PIN_VOL_ADC);
  uint8_t v = (uint8_t)map(raw, 0, 4095, 0, 100);
  if (v != s_lastVolCmd) {
    s_lastVolCmd = v;
    walleI2sAudioSetVolume(v);
  }
}

static bool triggerDown(void) {
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
  Serial.println(F("\nSlime Blower v2.1 (SD + I2S)"));

  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  mosfetsInit();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  g_sdOk = SD.begin(PIN_SD_CS, SPI, 20000000);

  WalleI2sPinConfig pins = {PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT, 1};
  g_audioOk = walleI2sAudioInit(&pins) && g_sdOk;
  if (g_audioOk) {
    walleI2sAudioSetVolume(80);
    s_lastVolCmd = 80;
    walleI2sAudioPlayFile("/audio/001.wav");
    Serial.println(F("SD + I2S audio OK"));
  } else {
    Serial.println(F("Audio init failed — LEDs + MOSFETs only"));
  }
}

void loop() {
  applyVolumeFromKnob();
  walleI2sAudioTick();

  int tank = readTankPercent();
  const bool critical = (tank < 10);
  const bool canBlow = (tank > 5);
  const bool blowing = triggerDown() && canBlow && !critical;

  setMosfetOutputs(blowing);
  updateLights(tank, blowing, critical);

  if (g_audioOk) {
    if (blowing && !s_wasBlowing) {
      walleI2sAudioPlayFile("/audio/002.wav");
    }
    if (!blowing && s_wasBlowing) {
      stopTrack();
    }
    s_wasBlowing = blowing;
  }

  delay(4);
}
