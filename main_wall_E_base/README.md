# Base Locomotion Node (“Brain”)

**ESP32-S3** module: **tank motors**, **servos**, **local TFT**, **Wi-Fi AP** (`WALL-E-Control`), **HTTP** web server, **ESP-NOW** receiver for the master controller, **vision** and **audio** satellite packets, **dock** homing and autonomous docking, **node health** aggregation.

## Purpose

- Execute **motor** commands from ESP-NOW; **fail-safe** stop if commands stop.
- Host **LROS**-related pages when integrated in `web_server.cpp`.
- Fuse **dock beacon**, **IR TX** alignment, **VL53L1X** approach, **dock beam** sensors.
- Forward **telemetry** to the master controller.

## Hardware (typical)

- **MCU:** ESP32-S3 DevKit-class (N16R8 or similar).
- **Motors:** L298N dual H-bridge (or project-specific).
- **Servos:** PCA9685 over I2C (see `servo_manager`).
- **Sensing:** VL53L1X ToF, rear IR beam, **front IR TX** on GPIO **21** / **38** (modulated for dock TSOP receivers), IMU, optional sonar/GPS when enabled in code.
- **Display:** ST7735/ST7789 TFT (`display_manager`).

**Pin map:** Always treat **`main/*.h` and `dock_sensors.h`** as authoritative — do not rely only on this README.

## Dependencies

- PlatformIO env **`wall_e_brain_s3`** (see `platformio.ini`).
- `lib_extra_dirs = ../lib` for **walle_emotion_pose** and event stubs.
- **Pololu VL53L1X**, **Adafruit PWM Servo**, **Adafruit GFX**, **MPU6050**, **TinyGPSPlus**, etc.

## Build & flash

```bash
cd main_wall_E_base
pio run -e wall_e_brain_s3
pio run -e wall_e_brain_s3 -t upload
```

**Arduino IDE:** See [ARDUINO_IDE_QUICK_START.md](ARDUINO_IDE_QUICK_START.md) — root `main.ino` delegates to `main/main.ino`.

## Node ID / MAC

- **Node health:** `WALLE_NODE_BASE` (`0`) in [main/node_health_protocol.h](main/node_health_protocol.h).
- **ESP-NOW:** Base is the **hub** for many packet types; ensure **same Wi-Fi channel** as peers.

## Communication with master controller

- **Receives:** **ControlPacket** (drive, flags, actions, servo targets).
- **Sends:** **TelemetryPacket** (battery, autonomy, safety, etc.).
- Definitions: [../wall_e_master_controller/protocol.h](../wall_e_master_controller/protocol.h) (shared shapes).

## Dock / IR

- **Dock protocol:** [main/dock_protocol.h](main/dock_protocol.h) — keep in sync with `dock_station/`.
- **Autonomous docking:** [main/AUTONOMOUS_DOCKING.md](main/AUTONOMOUS_DOCKING.md).
- **IR:** Transmitters use **LEDC ~38 kHz**; alignment hints come from dock beacon **`ir_align_hint`** when ESP-NOW receives dock packets.

## Calibration

- **Battery ADC** — `battery_monitor` scale/offset.
- **Motor trim** — `motor_control` if present.
- **VL53** — I2C address and mount orientation.
- **Dock** — ToF thresholds in `dock_sensors` / autonomous docking headers.

## OTA

- Web update: **`http://192.168.4.1/update`** (AP mode).  
- See root [OTA_README.md](../OTA_README.md).

## Related

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../README.md](../README.md)  
