# Audio Node (`audio_esp`)

Dedicated **ESP32-S3** sketch for the WALL-E **audio** subsystem: playback, voice, and ESP-NOW coordination with the base. Uses **DFRobot DFPlayer Mini** (see `platformio.ini`) and shared headers from **`../wall_e_audio`**.

## Purpose

- Offload audio from the base so motor RT and sound do not share one core’s worst-case latency.
- Emit **audio-related** ESP-NOW packets / status (see `audio_protocol.h`, `espnow_manager`).

## Hardware

- **Board:** `esp32-s3-devkitc-1` (see `platformio.ini`).
- **Audio:** DFPlayer Mini + SD card, or project-specific wiring documented in `audio_esp.ino`.

## Dependencies

- PlatformIO, `espressif32`, Arduino.
- **DFRobotDFPlayerMini** library.
- **Include path:** `-I ../wall_e_audio` for shared protocol headers.

## Build & flash

```bash
cd audio_esp
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload
```

## Node ID / MAC

- **Node health:** `WALLE_NODE_AUDIO` (`2`) — see [node_health_protocol.h](node_health_protocol.h) (copy; keep in sync with base).

## Communication with master controller

- **Indirect:** Master talks to **base**; base may forward or subscribe to audio node traffic via ESP-NOW.
- **Channel:** Must match the **base Wi-Fi channel** for reliable ESP-NOW.

## Calibration

- **DFPlayer:** UART baud (default 9600), busy pin, volume curve.
- **SD card:** File naming and folder layout per DFPlayer docs.

## Related

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../lib/walle_audio_events/walle_audio_events.h](../lib/walle_audio_events/walle_audio_events.h)  
