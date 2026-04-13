# WALL-E + EVE System Overview

This repository is a multi-node ESP32 stack: **base brain** (`main_wall_E_base`), **audio/voice board** (`audio_esp`), **EVE companion** (`eve`), plus dock, vision, and CYD controller nodes.

## Discovery and presence

Node online state continues to use ESP-NOW `WalleNodeHealthPacket_t` (`node_health_protocol.h`) for **base, master (CYD), audio, dock, vision**. **EVE** is not on ESP-NOW; presence is derived from the **UART bridge** (`eve_uart_bridge`) with the same timeout semantics as `/api/eve/status`.

Derived concepts (implemented in firmware telemetry and logs):

| Concept | Source |
|--------|--------|
| BASE_ONLINE | Always local |
| AUDIO_ONLINE | `nodeHealthIsOnline(WALLE_NODE_AUDIO)` |
| VISION_ONLINE | `nodeHealthIsOnline(WALLE_NODE_VISION)` |
| DOCK_ONLINE | Dock beacon / health slot |
| CONTROLLER_ONLINE | `nodeHealthIsOnline(WALLE_NODE_MASTER)` |
| EVE_ONLINE | `eveUartBridgeIsLinkUp()` |
| SHARED_VOICEBOX_ACTIVE | `sharedVoiceboxIsShared()` |

## New protocol headers (`/protocols`)

Shared C headers (also copied into `main_wall_E_base/main/` for Arduino IDE):

- `voicebox_protocol.h` — `WalleVoiceboxCmdPacket_t` (magic `VBXC`), versioned.
- `menu_protocol.h` — `WalleAudioUiTelemPacket_t` (magic `WAUI`), menu pages, UI events, pair requests.
- `relationship_protocol.h` — bond struct + Preferences namespace.
- `memory_protocol.h` — event type IDs and logical filenames for logs.

## Living core (base)

| Module | Role |
|--------|------|
| `memory_manager` | LittleFS append log + voicebox snapshot file |
| `relationship_manager` | Persistent bond counters; EVE attach/detach logging |
| `shared_voicebox_manager` | Chooses solo vs shared mode, sends `VBXC` to audio, routes pair cues to EVE UART |
| `autonomy_manager` | Lightweight behavior label for telemetry (works alongside existing autonomy stack) |
| `audio_ui_telemetry` | Last WAUI packet from audio ESP |
| `telemetry_manager` | Single JSON snapshot for HTTP + WebSocket |
| `websocket_manager` | WebSocket server on **port 81** (JSON broadcast ~2 Hz when clients connected) |

HTTP: `GET /api/living/telemetry` returns the same schema as WebSocket payloads.

## Audio ESP

- **DFPlayer** remains local for low latency.
- **Four buttons** on GPIO **9–12** (see `audio_esp/pins.h`).
- **6 s hold** of **BTN1+BTN2** toggles menu mode (CYD-style layered navigation).
- **WAUI** packets to base carry menu page, combo progress, and **pair_request** for EVE follow-up cues.

## EVE

- `eve_behavior_manager` — reacts to `MSG_PLAY_SOUND` (eyes + `audioPlayTrack`).
- `eve_attachment_manager` — optional `EVE_PRESENT_PIN`.
- `eve_status_manager` — compact JSON status string helper.

## Build notes

- Base `platformio.ini` adds `links2004/WebSockets` and `-I ../protocols`.
- Audio `platformio.ini` adds `-I ../protocols`.
- Place DFPlayer MP3s **010.mp3–016.mp3** on the audio SD card to match `audio_esp/config.h` track IDs.

## Validation checklist

1. **Boot** — Serial tags `[MEMORY]`, `[BOND]`, `[VOICE]`, `[WS]` as modules init.
2. **Button response** — `[BTN]` lines; tracks 10–13 play on audio ESP.
3. **6 s menu hold** — combo `%` in telemetry; `[MENU] Entered`; enter/exit cues 14/15.
4. **Record** — error track 13 + logs; never enables recording hardware.
5. **Menu navigation** — `menu_page` changes in `/api/living/telemetry`.
6. **Voice box solo** — EVE UART disconnected → `voicebox_mode` = `SOLO_WALLE`.
7. **Shared voice box** — EVE UART linked → `SHARED_WALLE_EVE`; `VBXC` updates audio; PLAY may trigger EVE track via UART.
8. **EVE attach** — bond counters increment; `[BOND]` log.
9. **EVE disconnect** — `[VOICE] EVE offline, falling back to SOLO_WALLE`.
10. **Docking / low battery** — reflected in telemetry `dock_ir`, `battery_pct`, behavior state.
11. **Persistence** — Preferences bond keys; `/walle_voicebox.json` on LittleFS when mounted.
12. **WebSocket** — connect to `ws://<ap-ip>:81/`; receive periodic JSON.
13. **Failsafe** — estop and `unifiedAutonomySafetyActive()` skip EVE pair cues.
