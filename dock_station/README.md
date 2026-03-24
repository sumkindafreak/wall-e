# Dock Station (Smart Charging Crate)

**ESP32-S3** fixed install: **charge control** (MOSFET + **ACS712**), **dock presence** (VL6180 ToF, optional IR break-beam, obstacles), **IR alignment receivers**, **arrow indicators**, **NeoPixel + TFT**, **ESP-NOW beacon** for homing, optional **home Wi-Fi**, and **OTA**.

---

## Purpose

- Run a **charging state machine** (not docked → idle → charging → charged / fault) with **current-based** thresholds.
- Broadcast **`DockBeaconPacket_t`** at **10 Hz** when ESP-NOW is active (`ESPNOW_BEACON_INTERVAL_MS` = 100 ms in [dock_config.h](dock_config.h)) — includes **`ir_align_hint`** when IR guidance runs.
- Provide **visual alignment** via arrow MOSFETs driven from **`dock_alignment`** / **`dock_ir_guidance`**.
- Optional **call WALL-E** switch and light sequences.

---

## Hardware summary

| Area | Config keys (see [dock_config.h](dock_config.h)) |
|------|---------------------------------------------------|
| Charge enable | `PIN_MOSFET_GATE` |
| Current sense | `PIN_ACS712_ADC` |
| VL6180 ToF | `PIN_VL6180_SDA`, `PIN_VL6180_SCL` |
| Alignment IR | `PIN_ALIGN_LEFT`, `PIN_ALIGN_RIGHT` (TSOP: **LOW** = IR seen) |
| Break-beam | `PIN_IR_BEAM` (separate from side alignment) |
| Arrows / UI | Arrow MOSFETs, NeoPixel data, TFT SPI, call switch |

**Important:** Pin tables in old wikis may be wrong. The **comment block at the top of `dock_config.h`** is the authoritative map for this firmware.

---

## Dependencies

- PlatformIO env **`dock_esp32`** — [platformio.ini](platformio.ini).
- **FastLED**, **Adafruit GFX**, **ST7735**, **VL6180X**, **BusIO**.

---

## Build and flash

```bash
cd dock_station
pio run -e dock_esp32
pio run -e dock_esp32 -t upload
```

**Arduino IDE:** Open `dock_station.ino`; sibling `.cpp`/`.h` files compile with the sketch.

---

## Node ID and addressing

| Item | Value |
|------|--------|
| **Dock ID** | `DOCK_ID` in `dock_config.h` (default `0x00000001`) — unique per physical dock if multiple exist |
| **Node health ID** | `WALLE_NODE_DOCK` = **3** — [node_health_protocol.h](node_health_protocol.h) |

---

## Communication responsibilities

| Peer | Protocol |
|------|----------|
| **Base** | ESP-NOW: beacon, `DOCK_CMD_*`, approach stage — [dock_protocol.h](dock_protocol.h) |
| **Master** | **Indirect** — only via base in stock firmware |

**Wi-Fi:** When `ENABLE_WIFI` is **0**, ESP-NOW beacon traffic may be disabled — base will not receive **`ir_align_hint`** over the air; align using dock **arrows** and local testing only.

---

## Configuration files

- **[dock_config.h](dock_config.h)** — Wi-Fi, thresholds, `USE_VL6180_TOF`, `DOCK_IR_LOST_TIMEOUT_MS`, NeoPixel map, `ENABLE_WIFI`.
- **[dock_protocol.h](dock_protocol.h)** — Must stay **byte-compatible** with [main_wall_E_base/main/dock_protocol.h](../main_wall_E_base/main/dock_protocol.h).

---

## Calibration

- **ACS712:** `ACS712_MV_PER_AMP`, `CURRENT_CALIB_*` in `dock_config.h`.
- **VL6180:** `VL6180_DOCK_MIN_MM` / `VL6180_DOCK_MAX_MM` for “robot in slot” band.
- **IR:** Robot must emit **modulated** IR (~38 kHz); adjust debounce in `dock_ir_guidance.cpp` if needed.

---

## OTA

- **ArduinoOTA** hostname **`wall-e-dock`** when on Wi-Fi — [../OTA_README.md](../OTA_README.md).

---

## Known quirks and limitations

- **`ENABLE_WIFI 0`** — Common for bring-up; no OTA/ESP-NOW beacon to base until Wi-Fi stack is enabled and configured.
- **Alignment vs beam** — Side **alignment IR** and **break-beam** are **different** sensors and software paths.
- **TFT/SPI** — Shares bandwidth with sensor polling; see serial for init failures.

---

## Extra documentation

- [TOF050C_VL6180.md](TOF050C_VL6180.md) — VL6180 tuning.

---

## See also

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../README.md](../README.md)  
