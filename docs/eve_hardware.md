# EVE companion node — hardware (ESP32-S3 N16R8)

This document matches the **EVE** firmware in `../eve/` and the same MCU class as **WALL-E** (`main_wall_E_base`): **ESP32-S3** module with **16 MB flash** and **8 MB OPI PSRAM** (often sold as **N16R8**).

## Module / board

- **MCU:** ESP32-S3 (Xtensa dual-core, 3.3 V logic).
- **Flash + PSRAM:** N16R8 — use a partition scheme that fits **16 MB flash**; enable **OPI PSRAM** in the build if your module uses it (see `eve/platformio.ini`).
- **Reference pinout:** ESP32-S3-DevKitC-1 style diagrams (e.g. UART1 on **GPIO17 TX / GPIO18 RX**) are a **reference** only. Your **full PCB** is the source of truth — update `eve/include/config.h` to match the routed signals.

## Hand link to WALL-E (primary): UART over pogo pins

**Minimum (4 pins):**

| Pogo | Signal | Notes |
|------|--------|--------|
| 1 | **GND** | Common ground; star at connector |
| 2 | **WALL-E TX → EVE RX** | One UART data line |
| 3 | **WALL-E RX ← EVE TX** | Other UART data line (full duplex) |
| 4 | *(optional)* **EVE_PRESENT** | Digital: faster “plugged in” before sync (see protocol doc) |

Default firmware UART mapping (DevKit-style, **override in `config.h` if your PCB differs**):

- **Debug / USB serial:** UART0 via USB bridge (typ. **GPIO43/44** — do not use for hand link if reserved for debug).
- **Hand link:** **UART1** — **TX = GPIO17**, **RX = GPIO18** (EVE side: RX receives from WALL-E TX, TX goes to WALL-E RX).

Baud rate: **115200 8N1** (see `EVE_UART_BAUD` in `config.h`).

## EVE peripherals (logical groups)

Pins are **placeholders** until your PCB is final — set real values once in `config.h`.

| Subsystem | Bus / interface | Notes |
|-----------|-----------------|--------|
| Dual TFT eyes | Shared **SPI** + separate **CS** per panel | One framebuffer strategy; PSRAM helps |
| ToF ×2 | **I²C** (shared SDA/SCL), address / XSHUT as needed | Short wires, pull-ups |
| Servo ×2 | **PWM** (LEDC) | Separate servo power, common GND |
| NeoPixel | **Single data GPIO**; level shifter if 5 V strip | Series resistor on data |
| DFPlayer | **UART** (often 9600 baud) | Use a **different** UART than the hand link |
| Hand ↔ WALL-E | **UART** | Dedicated; not shared with DFPlayer |

## Power

- If EVE is fed from WALL-E through the hand: add **fuse**, **bulk cap**, and a **3.3 V rail** for logic; isolate **servo** current from the MCU rail.
- If EVE is self-powered: **common GND** when mated is still mandatory.

## Firmware tree

- Source: **`eve/`** (PlatformIO). See **`eve/README.md`** for build/upload.
- Pins and feature flags: **`eve/include/config.h`** (must match your full PCB).

## Related docs

- Protocol framing: **`eve/include/eve_protocol.h`** (same message IDs as the base’s UART bridge parser).
- **Base (WALL-E brain):** optional **`main_wall_E_base/main/eve_uart_bridge.cpp`** — HTTP **`GET /api/eve/status`** for operator visibility when the hand link is wired.
