# WALL-E Vision Node

ESP32-S3 + OV2640 camera. Motion detection, clustering, centroid, object classification. Sends VisionPacket via ESP-NOW to Base Brain.

## Hardware

- **Board**: ESP32-S3 with PSRAM (e.g. ESP32-S3-DevKitC-1, XIAO ESP32-S3 Sense, Freenove)
- **Camera**: OV2640 (160×120 grayscale)

## Pin Configuration

Camera pins depend on your board. Edit `src/main.cpp` `s_camConfig` for your hardware:

| Board | Common pin sets |
|-------|-----------------|
| Freenove ESP32-S3 | See Freenove docs |
| XIAO ESP32-S3 Sense | Uses built-in cam, different config |
| Generic ESP32-S3 | Adjust PWDN, RESET, XCLK, SIOD, SIOC, D0-D7, VSYNC, HREF, PCLK |

## Build & Upload

```bash
cd vision_node
pio run
pio run -t upload
```

## Protocol

Sends `VisionPacket_t` via ESP-NOW broadcast (same channel as Base). Base must be on same WiFi channel.

## Parameters (motion_detect.h)

- `motionThreshold`: 20-30 (pixel diff)
- `minMotionPixels`: ~30
- `smoothFactor`: 0.4 (target smoothing)
- `occlusionTimeoutMs`: 500
