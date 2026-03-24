# Vision Node (PlatformIO — ESP32-S3 + Camera)

**ESP32-S3** with **OV2640** (or compatible) camera: **motion detection**, **clustering**, **centroid**, optional **object class**, and **ESP-NOW** transmission of **`VisionPacket_t`** to the base.

---

## Purpose

- Offload vision from the base; provide **target X/Y**, **bounding box**, **motion flag**, and **frame ID** for head/eye tracking or behaviour.

---

## Hardware

| Item | Detail |
|------|--------|
| **MCU** | ESP32-S3 with **PSRAM** strongly recommended |
| **Sensor** | OV2640 — **pin config is board-specific**; edit `src/main.cpp` camera structure |

---

## Dependencies

- PlatformIO — [platformio.ini](platformio.ini).
- ESP32 camera driver (Arduino-ESP32).
- **Same Wi-Fi channel** as the base AP for ESP-NOW (critical).

---

## Build and flash

```bash
cd vision_node
pio run
pio run -t upload
```

---

## Node ID and addressing

| Item | Value |
|------|--------|
| **Logical ID** | `WALLE_NODE_VISION` = **4** in node health documentation |
| **Health packets** | If implemented, must match `WalleNodeHealthPacket_t` layout |

---

## Communication responsibilities

| Peer | Protocol |
|------|----------|
| **→ Base** | `VisionPacket_t` — magic `VISION_MAGIC` — [include/vision_protocol.h](include/vision_protocol.h) |
| **Master** | **None** — vision does not target the CYD directly |

---

## Calibration

- **Motion:** `motion_detect.h` — `motionThreshold`, `minMotionPixels`, `smoothFactor`, `occlusionTimeoutMs`.
- **Camera:** Exposure, resolution, grayscale pipeline in `main.cpp` for your lighting.

---

## Canonical vs Arduino duplicate

This folder is the **PlatformIO** reference. A parallel **Arduino IDE** tree exists at **`../vision_node_arduino/`**. Pick **one** per deployment to avoid protocol drift; see root [README.md](../README.md).

---

## Known quirks and limitations

- **Frame rate** — Motion pipeline and ESP-NOW rate should be tuned to avoid CPU starvation.
- **Wi-Fi channel** — If ESP-NOW “goes silent,” verify router/AP channel matches the base.

---

## See also

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../main_wall_E_base/main/vision_behaviour.cpp](../main_wall_E_base/main/vision_behaviour.cpp)  
