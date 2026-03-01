// ==========================================================
// WALL-E AUDIO CORE — ESP32-S3
// Uses IAN audio engine (full implementation)
// SD + PCM5102 + RTC + 3 Buttons
// ==========================================================

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include "audio_engine.h"

// ==========================================================
// BUTTON PINS — IAN layout (free pins: 4, 13, 14)
// ==========================================================
#define BUTTON_RECORD   4
#define BUTTON_STOP    13
#define BUTTON_PREVIEW 14

// RTC I2C (optional — 21, 20)
#define RTC_SDA 21
#define RTC_SCL 20

// SD pins for scanner (same as ian_pins)
#define SD_CS   9
#define SD_SCK  12
#define SD_MOSI 10
#define SD_MISO 11

// ==========================================================
// I2C ADDRESS SCANNER
// ==========================================================
void scanI2C(int sda, int scl) {
    Serial.println(F("\n--- I2C Scanner ---"));
    Serial.printf("SDA=GPIO%d  SCL=GPIO%d\n", sda, scl);
    Wire.begin(sda, scl);
    delay(100);

    int count = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.printf("  [0x%02X] found", addr);
            if (addr == 0x68 || addr == 0x69) Serial.print(F(" (DS3231 RTC?)"));
            else if (addr == 0x3C || addr == 0x3D) Serial.print(F(" (OLED?)"));
            else if (addr == 0x40) Serial.print(F(" (PCA9685?)"));
            Serial.println();
            count++;
        }
    }
    if (count == 0) Serial.println(F("  No I2C devices found. Check wiring: SDA, SCL, GND, 3.3V"));
    else Serial.printf("  Total: %d device(s)\n", count);
    Serial.println(F("--- end scan ---\n"));
}

// ==========================================================
// SD DIAGNOSTIC
// ==========================================================
void probeSD() {
    Serial.println(F("\n--- SD Probe ---"));
    Serial.printf("CS=%d SCK=%d MOSI=%d MISO=%d\n", SD_CS, SD_SCK, SD_MOSI, SD_MISO);
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (SD.begin(SD_CS)) {
        Serial.println(F("  SD OK"));
        uint8_t cardType = SD.cardType();
        Serial.printf("  Type: %s\n", cardType == CARD_NONE ? "NONE" : (cardType == CARD_SD ? "SD" : "SDHC"));
        Serial.printf("  Size: %llu MB\n", SD.cardSize() / (1024 * 1024));
    } else {
        Serial.println(F("  SD FAILED — check: wiring, FAT32 format, CS/SCK/MOSI/MISO pins"));
    }
    Serial.println(F("--- end probe ---\n"));
}

// ==========================================================
// STATE
// ==========================================================
RTC_DS3231 rtc;
static bool rtcOK = false;
String lastAudioFile = "/track01.mp3";
const char* LOG_FILE = "/log.txt";
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;

// ==========================================================
// HELPERS
// ==========================================================
String getTimestamp() {
    if (rtcOK) {
        DateTime now = rtc.now();
        char buf[32];
        sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
        return String(buf);
    }
    char buf[16];
    sprintf(buf, "uptime:%lu", (unsigned long)(millis() / 1000));
    return String(buf);
}

void writeLog(String message) {
    File f = SD.open(LOG_FILE, FILE_APPEND);
    if (!f) return;
    f.println("[" + getTimestamp() + "] " + message);
    f.close();
    Serial.println("[" + getTimestamp() + "] " + message);
}

// ==========================================================
// RTC
// ==========================================================
void initRTC() {
    Wire.begin(RTC_SDA, RTC_SCL);
    if (!rtc.begin()) {
        Serial.println("RTC not found — continuing without RTC");
        rtcOK = false;
        return;
    }
    rtcOK = true;
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    Serial.println("RTC ready");
}

// ==========================================================
// BUTTONS
// ==========================================================
void handleButtons() {
    if (millis() - lastButtonPress < debounceDelay) return;

    if (digitalRead(BUTTON_RECORD) == LOW) {
        lastButtonPress = millis();
        writeLog("Record pressed");
    }

    if (digitalRead(BUTTON_STOP) == LOW) {
        lastButtonPress = millis();
        audioStop();
        writeLog("Stop pressed");
    }

    if (digitalRead(BUTTON_PREVIEW) == LOW) {
        lastButtonPress = millis();
        if (SD.exists(lastAudioFile)) {
            playAudioFromSD(lastAudioFile.c_str());
            writeLog("Preview: " + lastAudioFile);
        } else {
            audioPlay(1);
            writeLog("Preview: track01");
        }
    }
}

// ==========================================================
// SETUP
// ==========================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n===== WALL-E AUDIO CORE (IAN engine) =====");

    pinMode(BUTTON_RECORD, INPUT_PULLUP);
    pinMode(BUTTON_STOP, INPUT_PULLUP);
    pinMode(BUTTON_PREVIEW, INPUT_PULLUP);

    scanI2C(RTC_SDA, RTC_SCL);
    probeSD();

    initRTC();
    audioInit();

    if (audioState == AUDIO_ERROR) {
        Serial.println("Audio init failed — halting");
        while (true);
    }

    writeLog("System booted");
    Serial.println("Ready. Preview=track01, Stop=stop.");
}

// ==========================================================
// LOOP
// ==========================================================
void loop() {
    loopAudio();
    handleButtons();
    vTaskDelay(1);
}
