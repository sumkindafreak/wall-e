# Master Controller (CYD / Touch Operator Desk)

Handheld **operator interface** for WALL-E: touchscreen, drive abstraction, and **ESP-NOW** transmission of **ControlPacket** to the base. Receives **TelemetryPacket** for battery, autonomy state, and optional extras.

## Purpose

- Send **left/right** speed, **precision mode**, **behaviour/mood**, **actions** (scan, beep, dock go/cancel), **system flags** (E-stop, etc.).
- **Servo targets** array in the control packet for head/arms when enabled.
- Display telemetry and status from the base.

## Hardware

- **Typical board:** ESP32 with **CYD** 2432S028 (243×320 TFT + resistive touch) — see `platformio.ini` env `cyd_esp32_2432s028`.
- **Extras:** SX1509 IO expander, ADS1015, GPS (optional), per `lib_deps`.

## Dependencies

- PlatformIO + `espressif32` / Arduino.
- Libraries: **TFT_eSPI**, **XPT2046_Touchscreen**, **SparkFun SX1509**, **Adafruit ADS1X15**, **TinyGPSPlus** (see `platformio.ini`).
- `lib_extra_dirs = ../lib` for shared emotion pose stubs.

## Build & flash

```bash
cd wall_e_master_controller
pio run -e cyd_esp32_2432s028
pio run -e cyd_esp32_2432s028 -t upload
```

## Node ID / MAC

- **Node health:** `WALLE_NODE_MASTER` (`1`) in [node_health_protocol.h](node_health_protocol.h) — keep in sync with other copies in the repo.
- **ESP-NOW:** Peers must share the **Wi-Fi channel** with the base access point. No fixed MAC in source; pairing is dynamic unless you add explicit peer registration.

## Communication with base

- **Outbound:** Packed **ControlPacket** ([protocol.h](protocol.h)).
- **Inbound:** **TelemetryPacket** from base.
- Same channel as **WALL-E-Control** AP (e.g. `192.168.4.1` network).

## OTA

- **ArduinoOTA** on port **3232** when connected to the WALL-E AP — see root [OTA_README.md](../OTA_README.md).

## Calibration

- **Touch:** TFT_eSPI / touch calibration in `touch_input.cpp` / `profiles` as applicable to your board revision.
- **Display:** `profiles.h` / UI layout for 2432S028.

## Related

- [ARCHITECTURE.md](../ARCHITECTURE.md) — protocol overview  
- [../README.md](../README.md) — project overview  
