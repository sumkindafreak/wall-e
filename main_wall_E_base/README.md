# Base Locomotion Node (“Brain”)

The **ESP32-S3** central node: **tank drive**, **servos**, **local TFT**, **Wi-Fi softAP** (`WALL-E-Control`), **HTTP** web server (including LROS integration points), **ESP-NOW** hub for master, vision, audio, and dock traffic, plus **autonomous docking** and **node health** aggregation.

---

## Purpose

- Execute **motor** commands from the master (ESP-NOW) with **fail-safe** stop when commands time out.
- Host **REST/HTML** endpoints used by the web UI and OTA page.
- **Fuse** dock beacon RSSI, **IR TX** alignment, **VL53L1X** approach, and **rear IR beam** / ToF for docking behavior.
- **Aggregate** telemetry for the master and optional UI.

---

## Hardware (typical)

| Subsystem | Notes |
|-----------|--------|
| **MCU** | ESP32-S3 DevKit-class (e.g. N16R8) |
| **Motors** | L298N or compatible H-bridge |
| **Servos** | PCA9685 over I2C (`servo_manager`) |
| **Sensing** | VL53L1X ToF, rear dock IR beam, **front IR TX** on GPIO **21** / **38** (modulated — see `ir_beacon_receivers`), IMU; optional sonar/GPS when enabled in code |
| **Display** | ST7735/ST7789 (`display_manager`) |

**Authoritative pins:** `main/dock_sensors.h`, `main/ir_beacon_receivers.h`, and other `main/*.h` — **not** this README alone.

---

## Dependencies

- PlatformIO environment **`wall_e_brain_s3`** — [platformio.ini](platformio.ini).
- **`lib_extra_dirs = ../lib`** for `walle_emotion_pose` and event stubs.
- **Pololu VL53L1X**, **Adafruit PWM Servo**, **Adafruit GFX**, **MPU6050**, **TinyGPSPlus**, etc. (see `lib_deps`).

---

## Build and flash

```bash
cd main_wall_E_base
pio run -e wall_e_brain_s3
pio run -e wall_e_brain_s3 -t upload
```

**Arduino IDE:** [ARDUINO_IDE_QUICK_START.md](ARDUINO_IDE_QUICK_START.md) — root `main.ino` includes `main/main.ino`.

---

## Node ID and addressing

| Item | Value |
|------|--------|
| **Node health ID** | `WALLE_NODE_BASE` = **0** — [main/node_health_protocol.h](main/node_health_protocol.h) |
| **ESP-NOW** | Base acts as **RX hub**; peers must be on the **same channel** as this module’s AP |

---

## Communication responsibilities

| Peer | Protocol |
|------|----------|
| **Master** | `ControlPacket` in, `TelemetryPacket` out |
| **Vision** | `VisionPacket_t` (magic `VISION_MAGIC`) |
| **Dock** | `DockBeaconPacket_t`, dock commands — [main/dock_protocol.h](main/dock_protocol.h) |
| **Audio** | Audio-specific (see `audio_espnow` / protocols) |
| **Browser** | HTTP — see `web_server.cpp` |

---

## Docking and IR

- **Autonomous docking FSM:** [main/AUTONOMOUS_DOCKING.md](main/AUTONOMOUS_DOCKING.md).
- **IR transmitters:** `ir_beacon_receivers` uses **LEDC ~38 kHz** on ESP32 for TSOP-class dock receivers.
- **Alignment hints:** When the dock sends **`ir_align_hint`** on the beacon, the base autonomous **ALIGN** state can use it (requires Wi-Fi ESP-NOW path on dock).

---

## Calibration

- **Battery ADC** — `battery_monitor` scale/offset.
- **Motors** — Trim in `motor_control` if implemented.
- **VL53L1X** — I2C address, mounting, and field of view.
- **Dock thresholds** — `dock_sensors.h`, `autonomous_docking.h`.

---

## OTA

- **Web:** `http://192.168.4.1/update` when the AP is up — [../OTA_README.md](../OTA_README.md).

---

## Known quirks and limitations

- **`build_src_filter`** may exclude some `walle_emotion_pose` sources to avoid duplicate symbols — check `platformio.ini` before adding second copies.
- **Autonomy** — Some engine init may be commented in `main.ino` during bring-up; verify before expecting full auto behavior.
- **HTTP API** — Intended for **trusted LAN**; do not expose raw to the public internet without auth.

---

## See also

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../README.md](../README.md)  
