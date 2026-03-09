/**
 * WALL-E Vision Node - Arduino IDE
 * ESP32-S3-CAM N16R8 + OV2640
 * Motion detection, clustering, centroid, ESP-NOW to base brain.
 * SD card (rear slot): optional logging to /motion_log.csv
 *
 * Board: ESP32S3 Dev Module
 * Flash: 16MB, PSRAM: OPI PSRAM
 */

#include "esp_camera.h"
#include "vision_protocol.h"
#include "motion_detect.h"
#include "vision_espnow.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WebServer.h>
#include <WiFi.h>

#define FRAME_W    160
#define FRAME_H    120
#define XCLK_FREQ  20000000

/* ESP32-S3-CAM N16R8 + OV2640 (S3N16R8 pinout) */
camera_config_t camConfig = {
  .pin_pwdn = -1,
  .pin_reset = -1,
  .pin_xclk = 15,
  .pin_sccb_sda = 4,
  .pin_sccb_scl = 5,
  .pin_d7 = 16,
  .pin_d6 = 17,
  .pin_d5 = 18,
  .pin_d4 = 12,
  .pin_d3 = 10,
  .pin_d2 = 8,
  .pin_d1 = 9,
  .pin_d0 = 11,
  .pin_vsync = 6,
  .pin_href = 7,
  .pin_pclk = 13,
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

/* SD card - rear slot, 1-bit SDMMC. If your board uses different pins, change these. */
#define SD_LOG_ENABLE    1   /* 1 = log motion events to SD */
#define SD_PIN_CLK       39
#define SD_PIN_CMD       40
#define SD_PIN_D0        38
static bool s_sdOk = false;
static uint32_t s_sdLogInterval = 0;

/* WiFi: connect to base brain AP for web UI snapshot streaming */
#define WALLE_AP_SSID     "WALL-E-Control"
#define WALLE_AP_PASSWORD "walle1234"
static WebServer s_httpServer(80);
static uint32_t s_visionNodeIp = 0;

/* Onboard standard RGB LED - 3 GPIOs with PWM. (Avoid 1,3 = UART). Adjust for your board. */
#define LED_ENABLE       1
#define LED_PIN_R        21
#define LED_PIN_G        47
#define LED_PIN_B        48
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
  /* 1-bit mode for ESP32-S3-CAM N16R8 rear microSD slot */
  if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
    Serial.println("[SD] setPins failed");
    return false;
  }
  if (!SD_MMC.begin()) {
    Serial.println("[SD] Mount failed (no card or bad pins)");
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

  Serial.println("[Vision] Ready");
}

void loop() {
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
