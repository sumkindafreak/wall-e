# WALL-E Character Button & Menu System

## Host board choice: **Audio ESP**

**Why:** The audio module already owns the **DFPlayer**, ESP-NOW to the base, and real-time sound. Tactile “fake MP3 player” actions need **immediate** local playback; only higher-level state (menu page, voicebox mode, EVE pair requests) is mirrored to the base over ESP-NOW.

**Alternatives considered:** Base GPIO (far from speaker, more wiring), CYD (already a touch workflow, not life-size panel).

## GPIO (ESP32-S3 DevKitC-class wiring)

Defined in `audio_esp/pins.h`:

| Signal | GPIO | Notes |
|--------|------|--------|
| BTN1 (PLAY / menu UP) | 9 | INPUT_PULLUP, active LOW |
| BTN2 (STOP / menu DOWN) | 10 | INPUT_PULLUP |
| BTN3 (REWIND / SELECT) | 11 | INPUT_PULLUP |
| BTN4 (RECORD / BACK) | 12 | INPUT_PULLUP |

**Menu combo:** `PIN_MENU_COMBO_A` = BTN1, `PIN_MENU_COMBO_B` = BTN2 — hold both **6 s** to enter or exit menu (`MENU_COMBO_HOLD_MS` in `config.h`).

Avoid reusing pins already assigned: DFPlayer UART 17/18, mics 4/5, IR 6/7, status LED 8.

## Normal mode (fake MP3 player)

| Button | Action | DFPlayer track (`config.h`) |
|--------|--------|------------------------------|
| 1 | PLAY | `TRACK_CHAR_PLAY` (10) |
| 2 | STOP | `TRACK_CHAR_STOP` (11) |
| 3 | REWIND | `TRACK_CHAR_REWIND` (12) |
| 4 | RECORD | `TRACK_CHAR_RECORD_FAIL` (13) — **not a real recorder** |

Serial tags: `[BTN]`, `[AUDIO]`, `[AUTO]` on RECORD.

## Menu mode

After enter confirmation (`TRACK_MENU_ENTER_OK`, 14):

| Button | Role |
|--------|------|
| 1 | UP / previous page |
| 2 | DOWN / next page |
| 3 | SELECT (context) |
| 4 | BACK → STATUS |

Pages: STATUS, AUDIO TEST, EXPRESSIONS, DOCK STATUS, EVE LINK, VOICE BOX, MEMORY LOG, SYSTEM INFO, SAFE REBOOT.

**SAFE REBOOT:** first SELECT arms; second SELECT within 8 s calls `ESP.restart()` on the **audio ESP** only.

**Timeout:** `MENU_PAGE_TIMEOUT_MS` returns navigation to STATUS.

## ESP-NOW telemetry

`WalleAudioUiTelemPacket_t` (`menu_protocol.h`, magic `WAUI`) includes:

- `btn_mode`, `menu_page`, `combo_hold_pct`, `last_ui_event`, `menu_sel_idx`
- `voicebox_mode` (echo)
- `pair_request` (EVE follow-up routing on base)

## SD card audio assets

Add files **001.mp3–016.mp3** (or at minimum **010–016**) matching `audio_esp/config.h` naming convention (DFPlayer numeric order).
