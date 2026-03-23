# WALL-E Audio / Voice / Dock Sensor Brain — Architecture

## 1. Architecture Summary

The **Audio ESP** is a dedicated ESP32-S3 module that:

- **Senses:** Left/right mics, IR dock receivers, serial voice input
- **Plays:** All WALL-E sounds via DFPlayer
- **Reports:** Status, events, dock alignment, mic direction, faults
- **Receives:** Commands from the main Base ESP32-S3 over UART

**Critical rule:** The Audio ESP **never** controls drive motors. The Base ESP32-S3 is the only board that makes movement decisions. This module only senses, interprets, plays sound, and reports.

```
┌─────────────────────────────────────────────────────────────────┐
│                     AUDIO ESP (this module)                      │
├─────────────────────────────────────────────────────────────────┤
│  DFPlayer ◄──► Audio Player    │  Left Mic ──┐                   │
│  Left Mic  ──► Mic Manager ────┼─────────────┼──► MIC_DIR        │
│  Right Mic ──►                 │  Right Mic ─┘                   │
│  Voice UART ─► Voice Commands ─┼──► VOICE_CMD events             │
│  IR Left   ──► IR Dock         │  IR Right                       │
│  IR Right  ──► Receivers ──────┼──► DOCK_IR state                │
│                               │                                 │
│  Event Router ◄──► Comms Manager ◄──► UART ◄──► Base ESP32-S3   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. File-by-File Responsibility Map

| File | Responsibility |
|------|----------------|
| `audio_esp.ino` | Main entry, setup(), loop(), module init order |
| `config.h` | Feature flags, timing constants, track IDs, thresholds |
| `pins.h` | All GPIO and UART pin definitions |
| `system_state.h` | System mode enum, mode transitions, mode get/set |
| `audio_player.h` | DFPlayer init, play/stop/volume, busy state, priority |
| `mic_manager.h` | Left/right ADC read, smoothing, direction (LEFT/RIGHT/CENTER/UNKNOWN) |
| `voice_commands.h` | Serial voice parse, debounce, cooldown, emit VOICE_CMD events |
| `ir_dock_receivers.h` | Left/right IR read, debounce, states (NONE/LEFT/RIGHT/BOTH/UNSTABLE) |
| `comms_manager.h` | UART packet TX/RX, message queue, parse incoming, send outgoing |
| `event_router.h` | Route events to comms, handle incoming commands, mode changes |
| `diagnostics.h` | Print mode, mics, IR, audio, heartbeat, faults |
| `debug_log.h` | Serial debug macros (DEBUG_LOG, DEBUG_AUDIO, etc.) |

---

## 3. Pin-Planning Table

| Function | GPIO | Notes |
|----------|------|-------|
| **DFPlayer UART** | | |
| DFPlayer RX (ESP TX) | 17 | UART1 |
| DFPlayer TX (ESP RX) | 18 | |
| **Base link** | | ESP-NOW (no UART pins) |
| **Voice UART** (optional) | | |
| Voice RX (ESP TX) | TBD | if external voice module |
| Voice TX (ESP RX) | TBD | |
| **Left Mic** | TBD | ADC1-capable (e.g. 4) |
| **Right Mic** | TBD | ADC1-capable (e.g. 5) |
| **IR Dock Left** | TBD | Digital input, pull-up |
| **IR Dock Right** | TBD | Digital input, pull-up |
| **Status LED** | TBD | Optional, output |

**Note:** Base link uses ESP-NOW (WiFi); no UART required. Channel 11. Base sends WalleAudioCommandPacket_t; Audio ESP sends WalleAudioMicTelemPacket_t and WalleAudioStatusPacket_t.

---

## 4. State Machine Summary

| State | Description | Transitions |
|-------|-------------|-------------|
| BOOT | Initializing hardware | → IDLE |
| IDLE | Ready, listening for commands | → LISTENING, PLAYING_AUDIO, DOCK_ASSIST, FAULT |
| LISTENING | Mic active, waiting for voice/events | → IDLE, VOICE_COMMAND, PLAYING_AUDIO |
| PLAYING_AUDIO | DFPlayer playing | → IDLE when done |
| DOCK_ASSIST | Dock mode, reporting IR alignment | → IDLE |
| VOICE_COMMAND | Processing voice command | → IDLE, PLAYING_AUDIO |
| FAULT | Error state (DFPlayer fail, etc.) | → IDLE on recover |

---

## 5. Packet Protocol Summary

**Format:** One message per line, terminated by `\n`. No JSON; simple `KEY:VALUE` or `KEY` text.

### Outgoing (Audio ESP → Base)

| Message | Description |
|---------|-------------|
| `HEARTBEAT` | Periodic alive ping |
| `VOICE_CMD:STOP` | Voice said stop |
| `VOICE_CMD:COME_HERE` | Voice said come here |
| `VOICE_CMD:GO_HOME` | Voice said go home |
| `VOICE_CMD:SLEEP` | Voice said sleep |
| `VOICE_CMD:WAKE` | Voice said wake |
| `MIC_DIR:LEFT` | Louder on left |
| `MIC_DIR:RIGHT` | Louder on right |
| `MIC_DIR:CENTER` | Balanced |
| `MIC_DIR:UNKNOWN` | No clear direction |
| `DOCK_IR:NONE` | No IR detected |
| `DOCK_IR:LEFT` | Left IR only |
| `DOCK_IR:RIGHT` | Right IR only |
| `DOCK_IR:BOTH` | Both IR |
| `DOCK_IR:UNSTABLE` | Rapid switching |
| `AUDIO_BUSY:1` | Playback started |
| `AUDIO_DONE` | Playback finished |
| `MODE:IDLE` | Current mode |
| `FAULT:DFPLAYER` | DFPlayer init/play failed |
| `PONG` | Response to PING |

### Incoming (Base → Audio ESP)

| Message | Action |
|---------|--------|
| `PLAY:STARTUP` | Play startup track |
| `PLAY:HELLO` | Play hello/ack track |
| `PLAY:ERROR` | Play error sound |
| `PLAY:ID` | Play track by ID (e.g. PLAY:3) |
| `STOP_AUDIO` | Stop playback |
| `SET_VOLUME:20` | Set volume 0–30 |
| `ENABLE_LISTENING` | Enter LISTENING mode |
| `DISABLE_LISTENING` | Return to IDLE |
| `ENTER_DOCK_MODE` | Enter DOCK_ASSIST |
| `EXIT_DOCK_MODE` | Leave DOCK_ASSIST |
| `PING` | Reply PONG |
| `REQ_STATUS` | Send full status/diagnostics |
