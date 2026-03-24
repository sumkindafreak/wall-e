# Vision Node (Arduino IDE layout)

Arduino-IDE–friendly packaging of the **vision** ESP32 camera + motion pipeline, aligned with **vision_protocol.h** for ESP-NOW to the base.

## Purpose

Same as [vision_node](../vision_node/README.md) for builders who prefer a single `.ino` + `.cpp` tree without PlatformIO.

## Hardware

- ESP32-S3 + OV2640 (board-specific pins in sources).

## Dependencies

- ESP32 Arduino core, camera driver.
- Match **vision_protocol.h** with `vision_node` and base.

## Flashing (Arduino IDE)

1. Open the sketch folder containing `vision_node_arduino.ino`.
2. Select the correct **ESP32-S3** board and **USB**.
3. Compile & upload.

## Node ID / MAC

- **Vision** identity is logical (`WALLE_NODE_VISION` in protocol docs); **MAC** is per device after flash.

## Communication

- **ESP-NOW** broadcast of **VisionPacket_t** — same channel as base.

## Calibration

- See [vision_node/README.md](../vision_node/README.md) motion parameters.

## Deprecation note

If you maintain **both** `vision_node/` and `vision_node_arduino/`, document which is **canonical** in the root [README.md](../README.md) to avoid drift.
