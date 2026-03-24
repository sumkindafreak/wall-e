# Dock Station (Smart Charging Crate)

**ESP32-S3** “home base” for WALL-E: **charging control**, **current sense**, **dock presence** (VL6180 ToF, optional IR beam, obstacles), **IR alignment receivers**, **arrow guides**, **NeoPixel + TFT status**, **ESP-NOW beacon** for homing, optional **home Wi-Fi** and **OTA**.

## Purpose

- Safe **MOSFET** charge enable with **ACS712** feedback and state machine (not docked → idle → charging → charged / fault).
- **Beacon** at ~10 Hz: `DockBeaconPacket_t` including **`ir_align_hint`** when IR guidance is active (see [dock_protocol.h](dock_protocol.h)).
- **IR alignment:** TSOP-style **receivers** on **`PIN_ALIGN_LEFT` / `PIN_ALIGN_RIGHT`** — **LOW = IR seen**; WALL-E carries **modulated IR TX** on its side.
- **Approach staging** from WALL-E (`DOCK_CMD_APPROACH_STAGE`) with sensor fallback after timeout.

**Authoritative pins:** [dock_config.h](dock_config.h) (summary comment block at top). Older pin tables in wiki or chat may be **wrong** — trust the header.

## Hardware (summary)

| Area | Typical |
|------|---------|
| MCU | ESP32-S3 (e.g. N16R8) |
| Charge | N-MOSFET on `PIN_MOSFET_GATE` |
| Current | ACS712 on `PIN_ACS712_ADC` |
| ToF | VL6180X I2C — `PIN_VL6180_SDA` / `PIN_VL6180_SCL` |
| Alignment IR | Left/right digital inputs (TSOP) |
| Break-beam | Optional `PIN_IR_BEAM` (separate from alignment) |
| User I/O | Call switch, arrow MOSFETs, NeoPixel data, TFT SPI |
| Wi-Fi | STA to home AP when configured (`WIFI_HOME_*` or NVS from WALL-E share) |

## Dependencies

- PlatformIO env **`dock_esp32`** — [platformio.ini](platformio.ini).
- **FastLED**, **Adafruit GFX**, **ST7735**, **VL6180X**, **BusIO**.

## Build & flash

```bash
cd dock_station
pio run -e dock_esp32
pio run -e dock_esp32 -t upload
```

**Arduino IDE:** Open `dock_station.ino` from this folder; ensure all `.cpp`/`.h` siblings are included (Arduino 2.x picks them up).

## Node ID / MAC

- **Dock ID:** `DOCK_ID` in `dock_config.h` (default `0x00000001`).
- **Node health:** `WALLE_NODE_DOCK` (`3`) — [node_health_protocol.h](node_health_protocol.h).

## Communication with master controller

- **Indirect:** Dock ↔ **base** via ESP-NOW beacon and commands; master does not attach to dock directly unless you add a custom path.
- **Beacon:** Base consumes RSSI + `dock_id` + **`ir_align_hint`** for alignment FSM.

## Configuration

- **`dock_config.h`:** Wi-Fi, thresholds, `ENABLE_WIFI`, `USE_VL6180_TOF`, `DOCK_IR_LOST_TIMEOUT_MS`, NeoPixel layout.
- **`dock_protocol.h`:** Must match [main_wall_E_base/main/dock_protocol.h](../main_wall_E_base/main/dock_protocol.h) for packet sizes.

## Calibration

- **ACS712:** `ACS712_MV_PER_AMP`, `CURRENT_CALIB_*` in `dock_config.h`.
- **VL6180:** `VL6180_DOCK_MIN_MM` / `VL6180_DOCK_MAX_MM` for “robot present” band.
- **IR alignment:** Ensure **38 kHz** IR from robot; adjust debounce in `dock_ir_guidance.cpp` if needed.

## OTA

- **ArduinoOTA** hostname `wall-e-dock` when Wi-Fi connected — see [../OTA_README.md](../OTA_README.md).

## Extra docs

- [TOF050C_VL6180.md](TOF050C_VL6180.md) — VL6180 tuning notes.

## Related

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../README.md](../README.md)  
