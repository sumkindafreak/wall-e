# Vision Node (PlatformIO — ESP32-S3 + Camera)

**ESP32-S3** with **OV2640** (or compatible) camera: **motion detection**, **clustering**, **centroid**, optional **object class**, and **ESP-NOW** broadcast of **VisionPacket_t** to the base.

## Purpose

- Offload vision from the base; send **target X/Y**, **bbox**, **motion flag**, **frame ID** for head/eye tracking or behaviour.

## Hardware

- **MCU:** ESP32-S3 with **PSRAM** recommended.
- **Sensor:** OV2640; **pin config** is board-specific — edit `src/main.cpp` camera config struct.

## Dependencies

- PlatformIO (`vision_node/platformio.ini`).
- ESP32 camera driver (Arduino-esp32 bundled).
- Same **Wi-Fi channel** as base AP for ESP-NOW.

## Build & flash

```bash
cd vision_node
pio run
pio run -t upload
```

## Node ID / MAC

- **Node health:** `WALLE_NODE_VISION` (`4`) — base registry merges health from `WalleNodeHealthPacket_t` when vision node also sends health (if implemented).

## Communication with master controller

- **Direct path:** Typically **none** — vision → **base** only.
- **Indirect:** Base may expose vision-derived state to **telemetry** or **web UI**; master controller reads **base telemetry**.

## Protocol

- **VisionPacket_t** — [include/vision_protocol.h](include/vision_protocol.h), magic `VISION_MAGIC`.
- Packet size: `VISION_PACKET_SIZE`.

## Calibration

- **Motion:** `motion_detect.h` — `motionThreshold`, `minMotionPixels`, `smoothFactor`, `occlusionTimeoutMs`.
- **Camera:** Exposure, resolution, and grayscale path in `main.cpp` for your lighting.

## Alternative sketch

- **Arduino-style** twin: `../vision_node_arduino/` — use **one** canonical build per deployment to avoid confusion.

## Related

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../main_wall_E_base/main/vision_behaviour.cpp](../main_wall_E_base/main/vision_behaviour.cpp)  
