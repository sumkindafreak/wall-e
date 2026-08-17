/**
 * WALL-E Vision Node - ESP32-S3 + OV2640
 * Motion detection, clustering, centroid, ESP-NOW to Base Brain.
 * Wi-Fi: connects to WALL-E-Control AP, serves /snapshot for WebUI.
 */

#include <Arduino.h>
#include "esp_camera.h"
#include "vision_protocol.h"
#include "motion_detect.h"
#include "vision_espnow.h"
#include "recognition_engine.h"
#include <WebServer.h>
#include <WiFi.h>

#define WALLE_AP_SSID     "WALL-E-Control"
#define WALLE_AP_PASSWORD "walle1234"

#define FRAME_W    160
#define FRAME_H    120
#define XCLK_FREQ  20000000

static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 5000;
static constexpr uint32_t WIFI_CONNECT_POLL_MS = 250;

/* ESP32-S3-CAM N16R8 + OV2640 (S3N16R8 pinout) */
static camera_config_t s_camConfig = {
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

static MotionDetect s_motion;
static uint8_t* s_prevFrame = nullptr;
static uint32_t s_frameCount = 0;
static WebServer s_httpServer(80);
static uint32_t s_visionNodeIp = 0;
static uint32_t s_lastRecognMs = 0;

static bool camInit(void) {
  esp_err_t err = esp_camera_init(&s_camConfig);
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

static void handleSnapshot() {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) {
    s_httpServer.send(500, "text/plain", "No sensor");
    return;
  }

  // Temporarily switch to JPEG/QVGA for a useful WebUI still image, then put
  // the camera straight back into the low-cost grayscale mode used by motion
  // and recognition processing.
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

static void wifiInit() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.begin(WALLE_AP_SSID, WALLE_AP_PASSWORD);

  Serial.print("[Vision] Connecting to WALL-E-Control");
  const uint32_t startedMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(WIFI_CONNECT_POLL_MS);
    Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED) {
    s_visionNodeIp = (uint32_t)WiFi.localIP();
    Serial.printf("\n[Vision] WiFi OK %s ch=%d\n",
                  WiFi.localIP().toString().c_str(), WiFi.channel());

    s_httpServer.on("/snapshot", HTTP_GET, handleSnapshot);
    s_httpServer.on("/health", HTTP_GET, []() {
      String body = "{\"ok\":true,\"node\":\"wall-e-vision\",\"ip\":\"";
      body += WiFi.localIP().toString();
      body += "\",\"channel\":";
      body += String(WiFi.channel());
      body += "}";
      s_httpServer.send(200, "application/json", body);
    });
    s_httpServer.begin();
    Serial.println("[Vision] HTTP snapshot server ready: /snapshot /health");
  } else {
    // Do not leave the radio endlessly scanning for an AP. ESP-NOW is the
    // primary robot link and vision_espnow.cpp will pin that fallback to the
    // WALL-E channel-11 contract.
    WiFi.disconnect(false);
    s_visionNodeIp = 0;
    Serial.println("\n[Vision] Base AP not found; snapshot disabled, ESP-NOW remains available");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[Vision] WALL-E Vision Node");

  if (!camInit()) {
    Serial.println("[Vision] HALT - no camera");
    while (1) delay(1000);
  }

  // Bring up the Base AP link before ESP-NOW so both services share the same
  // physical Wi-Fi channel. The previous code defined wifiInit() but never
  // called it, so /snapshot could never be served.
  wifiInit();

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

  s_prevFrame = (uint8_t*)malloc(FRAME_W * FRAME_H);
  if (!s_prevFrame) {
    Serial.println("[Vision] HALT - no prev frame");
    while (1) delay(1000);
  }
  memset(s_prevFrame, 0, FRAME_W * FRAME_H);

  Serial.println("[Vision] Ready");
}

void loop() {
  const uint32_t now = millis();

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->len < (size_t)(FRAME_W * FRAME_H)) {
    if (fb) esp_camera_fb_return(fb);
    if (s_visionNodeIp) s_httpServer.handleClient();
    delay(10);
    return;
  }

  motionDetectProcess(&s_motion, fb->buf, s_prevFrame);
  memcpy(s_prevFrame, fb->buf, FRAME_W * FRAME_H);
  esp_camera_fb_return(fb);

  VisionPacket_t pkt = {};
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
      } else if (fr) {
        esp_camera_fb_return(fr);
      }
      sens->set_pixformat(sens, PIXFORMAT_GRAYSCALE);
      sens->set_framesize(sens, FRAMESIZE_QQVGA);
      s_lastRecognMs = now;
    }
  }

  recognitionApplyLockSmoothing(&pkt, FRAME_W, FRAME_H);

  visionEspNowSend(&pkt);
  if (s_visionNodeIp) s_httpServer.handleClient();

  s_frameCount++;
  if (s_frameCount % 50 == 0) {
    Serial.printf(
      "[Vision] F=%lu motion=%d x=%d y=%d sz=%u class=%u col=%u ev=%u dist=%u\n",
      (unsigned long)s_frameCount, (int)pkt.motionDetected, (int)pkt.targetX,
      (int)pkt.targetY, (unsigned)pkt.objectSize, (unsigned)pkt.objectClass,
      (unsigned)pkt.colourId, (unsigned)pkt.visionEvent,
      (unsigned)pkt.distanceBand);
  }

  delay(5);
}
