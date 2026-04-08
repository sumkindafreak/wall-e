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
| **SD card (SPI)** | **CS** GPIO **5**, **MOSI** 23, **MISO** 19, **SCK** 18 — see [sd_manager.h](sd_manager.h). Initialized in [wall_e_master_controller.ino](wall_e_master_controller.ino) after TFT init; **FAT32** recommended. |

---

## SD card (persistent storage)

The firmware mounts an SD card when present and uses it as the backing store for **macros**, **animations**, **profile import/export**, **story memory**, and **buffered logs** under **`/wall_e/`** (see `sd_manager.cpp`).

| Topic | Detail |
|-------|--------|
| **Boot** | `sdInit()` runs after `tft.init()` (up to **3** mount retries). If mount fails, the controller continues **without** SD; `sdIsAvailable()` is false. |
| **Logging** | `sdLog()` / `sdLogFlush()`; `sdUpdate()` is called from the main loop for periodic log flush. |
| **Macros** | `macroInit()` runs after SD init; record/save/load uses SD when available. |
| **System status** | `system_status` debug strings can include **`SD_OK`** / **`SD_--`** when `systemStatusFormatDebug()` is used. |

The **dev console** (`dev_console.*`) can draw SD status when integrated into the UI; it is optional and not required for SD to work.

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
