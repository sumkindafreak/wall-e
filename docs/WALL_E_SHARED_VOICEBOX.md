# Shared Voice Box (WALL-E + EVE)

## Modes

| Name | Value | Meaning |
|------|-------|---------|
| `VOICEBOX_SOLO_WALLE` | 0 | Only WALL-E-side character audio policy |
| `VOICEBOX_SHARED_WALLE_EVE` | 1 | Dual-character personality; EVE may answer UART cues |

The base decides the mode from **EVE UART link** (`eveUartBridgeIsLinkUp()`). It pushes state to the audio ESP with `WalleVoiceboxCmdPacket_t` (magic **VBXC**, see `voicebox_protocol.h`) about every **480 ms** via `audioEspNowSendVoiceboxCmd()`.

Fields:

- `eve_audio_ok` — base can reach EVE (UART). Audio still plays if zero; shared **pair** routing is suppressed.
- `bond_strength` — 0–255 from `relationship_manager` (Preferences-backed).

## Pair requests (`pair_request` in WAUI)

When the operator presses character buttons or uses menu tests, the audio ESP may set `pair_request` so the base can fire **MSG_PLAY_SOUND** to EVE without stalling DFPlayer:

| Request | Typical EVE track (configurable on EVE SD) |
|---------|---------------------------------------------|
| `EVE_PLAY_ACK` | 3 |
| `MODE_TRANSITION` | 2 |
| `EVE_STOP_SETTLE` | 4 |
| `EVE_REWIND_CONFUSED` | 5 |
| `EVE_RECORD_REACT` | 6 |
| `EVE_EXPRESSION` | 5 |

Mapping lives in `shared_voicebox_manager.cpp` (`routePairToEve`). Adjust to match your EVE MP3 layout.

## Safety

- `unifiedAutonomySafetyActive()` blocks EVE pair cues (WALL-E audio already local).
- UART failure does not stop DFPlayer; logs `[VOICE] UART send failed`.

## Persistence

- `memory_manager` writes `voicebox_state.json` on LittleFS (`/walle_voicebox.json`) when mode changes.
- Event log `WALLE_MEM_EV_VOICEBOX_MODE` records transitions.

## Operator API

- HTTP: `GET /api/living/telemetry` — `voicebox_mode`, `voicebox_shared`, `eve_uart`, bond, behavior, audio UI fields.
- WebSocket: `ws://<ip>:81/` — same JSON at ~2 Hz when clients are connected.

## Reconnect behavior

When EVE UART goes stale: base logs **`[VOICE] EVE offline, falling back to SOLO_WALLE`**, persists snapshot, and clears `eve_audio_ok` in the next **VBXC**. Audio ESP continues solo playback immediately.
