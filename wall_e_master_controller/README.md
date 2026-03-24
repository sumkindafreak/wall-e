# Master Controller (CYD Touch Desk)

The **handheld operator station** for WALL-E: resistive touch UI, drive abstraction, and **ESP-NOW** exchange of **`ControlPacket`** (out) and **`TelemetryPacket`** (in) with the base.

---

## Purpose

- Send normalized **left/right** speed, **precision** mode, **behaviour/mood**, **actions** (scan, beep, dock go/cancel), and **system flags** (E-stop, etc.).
- Optional **servo target** array for head/arms when the protocol and base build support it.
- Display **battery**, autonomy, and safety-related telemetry from the base.

---

## Hardware

| Item | Detail |
|------|--------|
| **Typical board** | ESP32 **CYD** 2432S028 — 243×320 TFT + resistive touch |
| **PlatformIO env** | `cyd_esp32_2432s028` ([platformio.ini](platformio.ini)) |
| **Optional peripherals** | SX1509 IO expander, ADS1015 ADC, GPS (see `lib_deps`) |

---

## Dependencies

- **PlatformIO** + `espressif32` / Arduino framework.
- Libraries: **TFT_eSPI**, **XPT2046_Touchscreen**, **SparkFun SX1509**, **Adafruit ADS1X15**, **TinyGPSPlus** (exact versions in `platformio.ini`).
- **`lib_extra_dirs = ../lib`** for shared emotion-pose stubs.

---

## Build and flash

```bash
cd wall_e_master_controller
pio run -e cyd_esp32_2432s028
pio run -e cyd_esp32_2432s028 -t upload
```

---

## Node ID and addressing

| Item | Value |
|------|--------|
| **Node health ID** | `WALLE_NODE_MASTER` = **1** — [node_health_protocol.h](node_health_protocol.h) (keep in sync with other copies in the repo) |
| **MAC address** | Factory Wi-Fi MAC per chip; no fixed MAC in source |
| **ESP-NOW channel** | Must match the **Wi-Fi channel** used by the **base access point** (`WALL-E-Control`) |

---

## Communication responsibilities

| Direction | Content |
|-----------|---------|
| **→ Base** | Packed `ControlPacket` — [protocol.h](protocol.h) |
| **← Base** | Packed `TelemetryPacket` |
| **Transport** | ESP-NOW (not HTTP) |

The master does **not** talk to the dock or vision nodes directly in the stock design.

---

## OTA

- **ArduinoOTA** on port **3232** when joined to the WALL-E AP — see [../OTA_README.md](../OTA_README.md).

---

## Calibration and setup

- **Touch** — Calibrate in `touch_input` / profile code for your CYD revision.
- **Display** — UI layout in `profiles` / `ui_draw` for 2432S028 resolution.

---

## Known quirks and limitations

- **Dual control:** If LROS (browser) also commands the base while the master is driving, define an operator policy to avoid conflicting commands.
- **ESP-NOW range** — Same as Wi-Fi PHY; obstacles and antenna orientation matter.
- **Telemetry** — Field availability depends on base firmware build (autonomy features may be disabled).

---

## See also

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../README.md](../README.md)  
