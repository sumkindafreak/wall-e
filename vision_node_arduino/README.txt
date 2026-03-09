WALL-E Vision Node - Arduino IDE
================================

Board: ESP32S3 Dev Module

Settings:
- Flash Size: 16MB (128Mb)
- PSRAM: OPI PSRAM
- Partition Scheme: Default 4MB with spiffs

Libraries: None required (esp_camera, WiFi, esp_now, SD_MMC come with ESP32 core)

SD card (rear slot):
- 1-bit SDMMC: CLK=39, CMD=40, D0=38 (default for ESP32-S3-CAM N16R8).
  If your board differs, edit SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0 in the .ino.
- When a card is present, motion events are appended to /motion_log.csv
  (millis, frameId, motion, targetX, targetY, objectClass, objectSize).
- Set SD_LOG_ENABLE to 0 to disable SD logging.

WiFi + Camera snapshot (for web UI):
- Vision node connects to base brain AP (WALL-E-Control / walle1234).
- Serves GET /snapshot (JPEG). Base brain web UI embeds the image.
- If WiFi fails, ESP-NOW motion data still works; snapshot disabled.

Onboard RGB LED (standard 3-pin):
- Uses PWM (LEDC) on GPIO 21=R, 47=G, 48=B by default. If your board uses different
  pins, edit LED_PIN_R, LED_PIN_G, LED_PIN_B in the .ino. Set LED_ENABLE to 0
  to disable. Status: red=init, blue=idle, green=motion detected.

Upload: File → Open → select vision_node_arduino.ino
       Tools → Board → ESP32S3 Dev Module
       Tools → Port → (your COM port)
       Sketch → Upload
