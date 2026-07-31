# Phase M — EVE SD storage, asset manager, and I2S audio

## Overview

EVE uses the **microSD card** as primary runtime storage. Audio output is **SD → WAV decoder → I2S → amplifier → speaker**. DFPlayer Mini is **not** used anywhere in this repository.

## SD layout

See `eve/sd_card_template/` for the recommended card structure:

| Path | Purpose |
|------|---------|
| `/config/audio.json` | Track index → WAV filename, master volume |
| `/config/robot.json` | Personality / behaviour weights |
| `/config/settings.json` | Runtime toggles (optional) |
| `/config/display.json` | Display preferences (optional) |
| `/audio/*.wav` | 16-bit PCM WAV assets |
| `/images/` | Future LVGL assets |
| `/animations/` | Future expression data |
| `/logs/` | Optional runtime logs |
| `/updates/` | Future OTA packages |

## Modules

| Module | Role |
|--------|------|
| `lib/walle_i2s_audio` | Shared non-blocking WAV + I2S engine (queue, pause, volume, callbacks) |
| `eve_asset_manager` | Mount SD, verify paths, load configs, resolve audio tracks |
| `audio_control` | Firmware façade: play/stop/pause/volume/status |

## Configuration

Set pins in `eve/include/config.h`:

- `EVE_SD_SPI_*` — SD card (SPI)
- `EVE_I2S_*` — speaker I2S (use port 1 if mic uses port 0)
- `EVE_ENABLE_AUDIO` — enable playback pipeline

Legacy UART `audioPlayTrack(n)` resolves `n` via `/config/audio.json` or falls back to `/audio/NNN.wav`.

## WALL-E audio node

`audio_esp/` uses the same `walle_i2s_audio` library with its own SD/I2S pins in `audio_esp/pins.h`.

## Expression / emotion

Eyes-only behaviour is unchanged: no mouth animation. Emotion states follow **BOOT → IDLE → CURIOUS → FOLLOW → INTERACT → HAPPY → THINKING → SLEEP** (see `PHASE_L_EVE_EMOTION_ENGINE.md`).
