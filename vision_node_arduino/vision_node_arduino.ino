/**
 * WALL-E Vision Node - Arduino IDE
 * ESP32-CAM (AI-Thinker) + OV2640
 * Motion detection, clustering, centroid, ESP-NOW to base brain.
 * SD card (rear slot): optional logging to /motion_log.csv
 *
 * Board: ESP32-CAM (AI-Thinker)
 * Flash/PSRAM: per module (typ. 4MB flash + PSRAM)
 */

#include "esp_camera.h"
#include "vision_protocol.h"
#include "motion_detect.h"
#include "recognition_engine.h"
#include "vision_espnow.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef WALLE_AP_SSID
#define WALLE_AP_SSID     "WALL-E-Control"
#endif
#ifndef WALLE_AP_PASSWORD
#define WALLE_AP_PASSWORD "walle1234"
#endif

#define FRAME_W    160
#define FRAME_H    120
#define XCLK_FREQ  20000000

/* ESP32-CAM AI-Thinker + OV2640 (matches Arduino ESP32 CAMERA_MODEL_AI_THINKER) */
camera_config_t camConfig = {
  .pin_pwdn = 32,
  .pin_reset = -1,
  .pin_xclk = 0,
  .pin_sccb_sda = 26,
  .pin_sccb_scl = 27,
  .pin_d7 = 35,
  .pin_d6 = 34,
  .pin_d5 = 39,
  .pin_d4 = 36,
  .pin_d3 = 21,
  .pin_d2 = 19,
  .pin_d1 = 18,
  .pin_d0 = 5,
  .pin_vsync = 25,
  .pin_href = 23,
  .pin_pclk = 22,
  .xclk_freq_hz = XCLK_FREQ,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_GRAYSCALE,
  .frame_size = FRAMESIZE_QQVGA,
  .jpeg_quality = 12,
  .fb_count = 1,
  .grab_mode = CAMERA_GRAB_LATEST,
};

MotionDetect s_motion;
uint8_t* s_prevFrame = nullptr;
uint32_t s_frameCount = 0;

/* SD card - ESP32-CAM on-board µSD (1-bit SDMMC using default pins). */
#define SD_LOG_ENABLE    1   /* 1 = log motion events to SD */
static bool s_sdOk = false;
static uint32_t s_sdLogInterval = 0;

/* WiFi: connect to base brain AP for web UI snapshot streaming (override via secrets.h) */
static WebServer s_httpServer(80);
static uint32_t s_visionNodeIp = 0;
static uint32_t s_lastRecognMs = 0;

/* Onboard status LED — ESP32-CAM has a single LED on GPIO33. */
#define LED_ENABLE       1
#define LED_PIN_R        33
#define LED_PIN_G        33
#define LED_PIN_B        33
#define LED_BRIGHTNESS   128
#define LEDC_FREQ        5000
#define LEDC_RES         8
static uint32_t s_ledLastMotion = 0;

bool camInit(void) {
  esp_err_t err = esp_camera_init(&camConfig);
  if (err != ESP_OK) {
    Serial.printf("[Vision] Camera init FAILED: 0x%x\n", err);
    return false;
  }
  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, (framesize_t)FRAMESIZE_QQVGA);
    s->set_pixformat(s, PIXFORMAT_GRAYSCALE);
  }
  Serial.println("[Vision] Camera OK (160x120 grayscale)");
  return true;
}

bool sdInit(void) {
  /* ESP32-CAM on-board SD: use default SD_MMC pins, 1-bit mode. */
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("[SD] Mount failed (no card or bad wiring)");
    return false;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[SD] No card attached");
    return false;
  }
  Serial.printf("[SD] OK type=%s size=%lluMB\n",
    cardType == CARD_MMC ? "MMC" : (cardType == CARD_SD ? "SDSC" : "SDHC"),
    SD_MMC.cardSize() / (1024 * 1024));
  return true;
}

void sdLogMotion(uint32_t frameId, uint8_t motion, int16_t x, int16_t y, uint8_t objClass, uint16_t size) {
  if (!s_sdOk || !SD_LOG_ENABLE) return;
  if (millis() - s_sdLogInterval < 1000) return;  /* max once per second */
  s_sdLogInterval = millis();
  File f = SD_MMC.open("/motion_log.csv", FILE_APPEND);
  if (!f) return;
  f.printf("%lu,%lu,%u,%d,%d,%u,%u\n", millis(), frameId, motion, x, y, objClass, size);
  f.close();
}

void ledSet(uint8_t r, uint8_t g, uint8_t b) {
#if LED_ENABLE
  ledcWrite(LED_PIN_R, r);
  ledcWrite(LED_PIN_G, g);
  ledcWrite(LED_PIN_B, b);
#endif
}

void handleSnapshot() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) { s_httpServer.send(500, "text/plain", "No sensor"); return; }
  s->set_framesize(s, (framesize_t)FRAMESIZE_QVGA);
  s->set_pixformat(s, PIXFORMAT_JPEG);
  camera_fb_t* fb = esp_camera_fb_get();
  s->set_pixformat(s, PIXFORMAT_GRAYSCALE);
  s->set_framesize(s, (framesize_t)FRAMESIZE_QQVGA);
  if (!fb || fb->len == 0) {
    if (fb) esp_camera_fb_return(fb);
    s_httpServer.send(500, "text/plain", "Capture failed");
    return;
  }
  s_httpServer.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void wifiInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WALLE_AP_SSID, WALLE_AP_PASSWORD);
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    s_visionNodeIp = (uint32_t)WiFi.localIP();
    Serial.printf("\n[Vision] WiFi OK %s\n", WiFi.localIP().toString().c_str());
    s_httpServer.on("/snapshot", HTTP_GET, handleSnapshot);
    s_httpServer.begin();
  } else {
    Serial.println("\n[Vision] WiFi failed, snapshot disabled");
  }
}

void ledUpdate(bool motionDetected) {
#if LED_ENABLE
  if (motionDetected) {
    s_ledLastMotion = millis();
    ledSet(0, LED_BRIGHTNESS, 0);   /* green = motion */
  } else {
    uint32_t age = millis() - s_ledLastMotion;
    if (age < 300) {
      ledSet(0, LED_BRIGHTNESS, 0);
    } else {
      ledSet(0, 0, LED_BRIGHTNESS / 2);  /* blue dim = idle */
    }
  }
#endif
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[Vision] WALL-E Vision Node");

  if (!camInit()) {
    Serial.println("[Vision] HALT - no camera");
    while (1) delay(1000);
  }

  motionDetectInit(&s_motion);
  motionDetectSetFrameSize(&s_motion, FRAME_W, FRAME_H);
  s_motion.diffBuffer = (uint8_t*)malloc(FRAME_W * FRAME_H);
  s_motion.diffBufferSize = FRAME_W * FRAME_H;
  if (!s_motion.diffBuffer) {
    Serial.println("[Vision] HALT - no diff buffer");
    while (1) delay(1000);
  }

  if (!visionEspNowInit()) {
    Serial.println("[Vision] ESP-NOW failed, continuing...");
  }
  wifiInit();

  s_sdOk = sdInit();
  if (!s_sdOk) Serial.println("[Vision] SD not available (logging disabled)");

  s_prevFrame = (uint8_t*)malloc(FRAME_W * FRAME_H);
  if (!s_prevFrame) {
    Serial.println("[Vision] HALT - no prev frame");
    while (1) delay(1000);
  }
  memset(s_prevFrame, 0, FRAME_W * FRAME_H);

#if LED_ENABLE
  ledcAttach(LED_PIN_R, LEDC_FREQ, LEDC_RES);
  ledcAttach(LED_PIN_G, LEDC_FREQ, LEDC_RES);
  ledcAttach(LED_PIN_B, LEDC_FREQ, LEDC_RES);
  ledSet(LED_BRIGHTNESS, 0, 0);  /* red = init */
  delay(200);
  ledSet(0, 0, LED_BRIGHTNESS);  /* blue = ready */
#endif

  Serial.println("[Vision] Ready (motion + lightweight recognition)");
}

void loop() {
  uint32_t now = millis();
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->len < (size_t)(FRAME_W * FRAME_H)) {
    if (fb) esp_camera_fb_return(fb);
    delay(10);
    return;
  }

  motionDetectProcess(&s_motion, fb->buf, s_prevFrame);
  memcpy(s_prevFrame, fb->buf, FRAME_W * FRAME_H);
  esp_camera_fb_return(fb);

  VisionPacket_t pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.magic = VISION_MAGIC;
  pkt.motionDetected = s_motion.motionDetected ? 1 : 0;
  pkt.targetX = s_motion.targetX;
  pkt.targetY = s_motion.targetY;
  pkt.objectSize = s_motion.objectSize;
  pkt.bboxWidth = s_motion.bboxWidth;
  pkt.bboxHeight = s_motion.bboxHeight;
  pkt.objectClass = s_motion.objectClass;
  pkt.frameID = s_motion.frameID;
  pkt.visionNodeIp = s_visionNodeIp;

  recognitionApplyMotionBasics(&pkt, &s_motion, FRAME_W, FRAME_H);

  if (now - s_lastRecognMs >= RECOGNITION_INTERVAL_MS) {
    sensor_t* sens = esp_camera_sensor_get();
    if (sens) {
      sens->set_pixformat(sens, PIXFORMAT_RGB565);
      sens->set_framesize(sens, FRAMESIZE_QQVGA);
      camera_fb_t* fr = esp_camera_fb_get();
      if (fr && fr->len >= (size_t)(FRAME_W * FRAME_H * 2)) {
        recognitionProcessRgbFrame(&pkt, fr->buf, FRAME_W, FRAME_H);
        esp_camera_fb_return(fr);
      } else {
        if (fr) esp_camera_fb_return(fr);
      }
      sens->set_pixformat(sens, PIXFORMAT_GRAYSCALE);
      sens->set_framesize(sens, FRAMESIZE_QQVGA);
      s_lastRecognMs = now;
    }
  }

  recognitionApplyLockSmoothing(&pkt, FRAME_W, FRAME_H);

  visionEspNowSend(&pkt);

  if (s_visionNodeIp) s_httpServer.handleClient();

  if (s_sdOk && pkt.motionDetected)
    sdLogMotion(pkt.frameID, pkt.motionDetected, pkt.targetX, pkt.targetY, pkt.objectClass, pkt.objectSize);

  ledUpdate(pkt.motionDetected);

  s_frameCount++;
  if (s_frameCount % 50 == 0) {
    Serial.printf("[Vision] F=%lu motion=%d x=%d y=%d sz=%u class=%u\n",
      s_frameCount, pkt.motionDetected, pkt.targetX, pkt.targetY,
      pkt.objectSize, pkt.objectClass);
  }

  delay(5);
}
