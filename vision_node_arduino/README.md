# Vision Node (Arduino IDE)

Arduino-IDE–friendly layout for the **vision** pipeline: ESP32-S3 + camera, **motion detection**, and **ESP-NOW** broadcast of **`VisionPacket_t`** to the base — same logical role as [vision_node](../vision_node/README.md) (PlatformIO).

---

## Purpose

- Same as PlatformIO vision node for users who prefer a single `.ino` entry point without PlatformIO.

---

## Hardware

- ESP32-S3 + OV2640 (board-specific pins in sources).

---

## Dependencies

- ESP32 Arduino core, camera driver.
- **`vision_protocol.h`** must match [vision_node](../vision_node/include/vision_protocol.h) and base expectations.

---

## Flashing (Arduino IDE)

1. Open the sketch folder containing `vision_node_arduino.ino`.
2. Select the correct **ESP32-S3** board and **USB** port.
3. Compile and upload.

---

## Node ID and addressing

| Item | Detail |
|------|--------|
| **Logical role** | Vision satellite — `WALLE_NODE_VISION` in docs |
| **MAC** | Per-device after flash |

---

## Communication responsibilities

- **ESP-NOW** — `VisionPacket_t` to base; **same RF channel** as base AP.

---

## Calibration

- See [vision_node/README.md](../vision_node/README.md) motion parameters.

---

## Maintenance policy

If both `vision_node/` and `vision_node_arduino/` are kept:

- **Document which is canonical** in the root README.
- **Change `vision_protocol.h` in one place** and copy to the other, or symlink, to avoid struct mismatch.

---

## See also

- [../vision_node/README.md](../vision_node/README.md)  
- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
