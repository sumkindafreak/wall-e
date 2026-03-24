# WALL-E Multi-Node Robotics Platform

[![License](https://img.shields.io/badge/license-project-lightgrey)](LICENSE)

Open-source, multi-controller robotics stack for a life-size WALL-E–style build: a **handheld master controller**, **base locomotion brain**, **audio**, **vision**, **charging dock**, and a **browser UI (LROS)**. Nodes communicate over **Wi-Fi** and **ESP-NOW** with a shared **node health** heartbeat.

**Repository:** [github.com/sumkindafreak/wall-e](https://github.com/sumkindafreak/wall-e)

---

## Project overview

WALL-E splits responsibilities across ESP32-class modules:

- **No single MCU runs everything.** The operator desk sends drive commands; the base executes motors and aggregates sensors; optional satellites handle sound and camera processing.
- **ESP-NOW** carries low-latency control and telemetry between devices on the same Wi-Fi channel.
- **Node health** packets (`WNHT`) let the base and UI see which peers are alive, charging, or faulted.
- **Docking** combines ESP-NOW beacons, IR alignment (dock receivers + robot transmitters), optional VL6180 ToF, and charge control.

For deeper design notes see [ARCHITECTURE.md](ARCHITECTURE.md). For contribution workflow see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## Node architecture (ASCII)

```
                    ┌─────────────────────────────┐
                    │   wall_e_master_controller  │
                    │   (CYD / touchscreen desk)   │
                    │   ESP32 — operator UI        │
                    └──────────────┬──────────────┘
                                   │ ESP-NOW
           ┌───────────────────────┼───────────────────────┐
           │                       │                       │
           ▼                       ▼                       ▼
   ┌───────────────┐       ┌───────────────┐       ┌───────────────┐
   │ main_wall_E_  │       │   audio_esp   │       │  vision_node  │
   │    base       │       │  (voice / FX) │       │ (camera / MD) │
   │ ESP32-S3      │◄─────►│   ESP32…      │       │  ESP32-S3     │
   │ tank + servos │       │               │       │  + OV2640     │
   └───────┬───────┘       └───────────────┘       └───────┬───────┘
           │ ESP-NOW                                      │
           │                                                │
           ▼                                                │
   ┌───────────────┐                                       │
   │  dock_station │◄──────────────────────────────────────┘
   │  Smart crate  │         (same RF channel / LAN as configured)
   │  charge + IR  │
   └───────────────┘

   ┌───────────────┐
   │    webui/     │  Static LROS assets (or served from base AP)
   │  (LROS HTML)  │
   └───────────────┘
```

---

## Subsystems

| Subsystem | Folder | Role |
|-----------|--------|------|
| **Master controller** | `wall_e_master_controller/` | Touch UI, drive abstraction, ESP-NOW control packets to base, receives telemetry. |
| **Base locomotion** | `main_wall_E_base/` | Motors, servos, display, Wi-Fi AP, web server, ESP-NOW RX, dock homing, autonomous docking, node health aggregation. |
| **Audio node** | `audio_esp/` | Separate ESP audio / voice path; ESP-NOW integration with base and protocols under `audio_esp/`. |
| **Vision node** | `vision_node/` (PlatformIO) or `vision_node_arduino/` | Camera, motion detection, `VisionPacket_t` to base via ESP-NOW. |
| **Dock station** | `dock_station/` | Charging MOSFET, current sense, VL6180 ToF optional, IR alignment receivers, NeoPixel/TFT, dock beacon + `ir_align_hint`. |
| **Web UI (LROS)** | `webui/` | Operator pages (`index.html`, `lros.js`, `lros.css`) — status, control, and integration with base when hosted. |

Module-specific READMEs: [wall_e_master_controller/README.md](wall_e_master_controller/README.md), [main_wall_E_base/README.md](main_wall_E_base/README.md), [audio_esp/README.md](audio_esp/README.md), [vision_node/README.md](vision_node/README.md), [dock_station/README.md](dock_station/README.md), [webui/README.md](webui/README.md).

---

## Communication

| Mechanism | Use |
|-----------|-----|
| **ESP-NOW** | Master → base **ControlPacket**; base → master **TelemetryPacket**; vision **VisionPacket_t**; audio/aux protocols; dock **DockBeaconPacket_t** / commands; **WalleNodeHealthPacket_t** (`WNHT`) heartbeats. |
| **Wi-Fi (STA/AP)** | Base exposes **WALL-E-Control** AP (e.g. `192.168.4.1`); home STA optional. Dock may join home Wi-Fi for beacon/OTA. |
| **Heartbeat / node health** | `WALLE_NODE_BASE`, `MASTER`, `AUDIO`, `DOCK`, `VISION` slots in [node_health_protocol.h](main_wall_E_base/main/node_health_protocol.h) (copies per node — keep in sync). |
| **HTTP** | Base web UI and OTA update page; static LROS can be opened locally or served. |

See [ARCHITECTURE.md](ARCHITECTURE.md) for protocol relationships and docking flow.

---

## Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE) **or** Arduino IDE 2.x with ESP32 core (see [main_wall_E_base/ARDUINO_IDE_QUICK_START.md](main_wall_E_base/ARDUINO_IDE_QUICK_START.md) for base).
- USB cable per board; common baud **115200**.

### Clone and build

```bash
git clone https://github.com/sumkindafreak/wall-e.git
cd wall-e
```

Per project:

```bash
cd main_wall_E_base && pio run -e wall_e_brain_s3
cd ../wall_e_master_controller && pio run -e cyd_esp32_2432s028
cd ../dock_station && pio run -e dock_esp32
# etc.
```

Shared code lives under [`lib/`](lib/README.md) — `lib_extra_dirs = ../lib` is already set for several projects.

### OTA

See **[OTA_README.md](OTA_README.md)** for `espota`, web update URL on the base, and `ota_build_all` scripts.

### MAC / pairing notes

- ESP-NOW uses **broadcast** or **registered peers** depending on sketch; ensure all nodes share the **same Wi-Fi channel** as the base AP when using ESP-NOW.
- After flashing a new ESP32, the **Wi-Fi MAC** changes — re-pair controller ↔ base if your workflow stores MACs.
- Dock beacon and commands use `dock_id` in [dock_protocol.h](dock_station/dock_protocol.h); set `DOCK_ID` in `dock_config.h` if you run multiple docks.

---

## Hardware overview

| Node | Typical MCU | Notable I/O |
|------|-------------|-------------|
| Master | ESP32 (e.g. CYD 2432S028) | TFT + touch, ESP-NOW |
| Base | ESP32-S3 | L298N or similar motors, servos (I2C PWM), VL53L1X ToF, IR beam, **IR TX** GPIO 21/38 for dock alignment |
| Audio | ESP32 variant | I2S / DFPlayer / project-specific |
| Vision | ESP32-S3 + PSRAM | OV2640, ESP-NOW TX |
| Dock | ESP32-S3 | Charge MOSFET, ACS712, VL6180 I2C (SDA/SCL in `dock_config.h`), **IR receivers** on alignment pins, NeoPixel |

**IR dock beacon:** Robot emits **modulated IR** (~38 kHz); dock uses **TSOP-style receivers** on left/right alignment GPIOs. Beacon includes **`ir_align_hint`** for closed-loop alignment when Wi-Fi/ESP-NOW is enabled on the dock.

*Authoritative pin maps are always in each project’s `*_config.h` / `platformio.ini` — do not rely only on this table.*

---

## Media placeholders

<!-- Add your assets under docs/media/ or link externally -->

| Asset | Placeholder |
|-------|-------------|
| Hero photo | `![WALL-E build](docs/media/hero.jpg)` |
| Master controller UI | `![CYD UI](docs/media/controller.jpg)` |
| Dock + base | `![Dock](docs/media/dock.jpg)` |
| Short demo GIF | `![Demo](docs/media/demo.gif)` |

---

## Roadmap

- [ ] Unify `node_health_protocol.h` copies via single include path or code generation
- [ ] Document vision node channel sync procedure in one place
- [ ] Optional WebSocket bridge for LROS ↔ base
- [ ] CI build matrix for `main_wall_E_base`, `dock_station`, `wall_e_master_controller`
- [ ] Expand integration tests on ESP-NOW packet sizes

---

## License

See repository license file if present; otherwise treat as project-specific until a SPDX license is added.

---

## Related docs

- [ARCHITECTURE.md](ARCHITECTURE.md) — design philosophy and protocols  
- [CONTRIBUTING.md](CONTRIBUTING.md) — branches, commits, adding nodes  
- [FOLDER_STRUCTURE.md](FOLDER_STRUCTURE.md) — proposed repo layout (non-binding)  
- [OTA_README.md](OTA_README.md) — over-the-air updates  
