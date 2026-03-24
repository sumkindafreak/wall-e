# Audio Node (`audio_esp`)

Dedicated **ESP32-S3** firmware for WALL-E **sound**: playback, voice, and **ESP-NOW** coordination with the base. Uses **DFRobot DFPlayer Mini** ([platformio.ini](platformio.ini)) and shared headers from **`../wall_e_audio`** when that path exists in your workspace.

---

## Purpose

- Offload audio from the base so motor control and audio decoding are not competing on the same critical path.
- Send/receive **audio-related** ESP-NOW traffic (see `audio_protocol.h`, `espnow_manager`).

---

## Hardware

| Item | Detail |
|------|--------|
| **Board** | `esp32-s3-devkitc-1` (see `platformio.ini` env `esp32-s3-devkitc-1`) |
| **Audio** | DFPlayer Mini + microSD, UART wiring per `audio_esp.ino` |

---

## Dependencies

- PlatformIO, `espressif32`, Arduino.
- **DFRobotDFPlayerMini** library.
- **Build flags:** `-I ../wall_e_audio` for shared protocol headers.

---

## Build and flash

```bash
cd audio_esp
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload
```

---

## Node ID and addressing

| Item | Value |
|------|--------|
| **Node health ID** | `WALLE_NODE_AUDIO` = **2** — [node_health_protocol.h](node_health_protocol.h) (keep in sync) |
| **Channel** | ESP-NOW requires the **same Wi-Fi channel** as the base AP |

---

## Communication responsibilities

| Path | Role |
|------|------|
| **↔ Base** | ESP-NOW; master does **not** speak to audio directly in the stock architecture |
| **Master** | Indirect — via base telemetry or future bridging |

---

## Calibration

- **DFPlayer:** UART baud (typically 9600), busy pin, volume curve.
- **SD card:** File naming (`0001.mp3` style) and folder layout per DFPlayer documentation.

---

## Known quirks and limitations

- **`wall_e_audio` path** — If the sibling folder is missing, add it or adjust `build_flags` includes.
- **Latency** — DFPlayer is not sample-accurate; gapless playback has limits.

---

## See also

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../lib/walle_audio_events/walle_audio_events.h](../lib/walle_audio_events/walle_audio_events.h)  
