# WALL-E Audio ESP

Dedicated **ESP32-S3** firmware for WALL-E **sound**: SD-card **WAV** playback over **I2S**, voice routing, and **ESP-NOW** coordination with the base.

## Hardware

| Subsystem | Notes |
|-----------|--------|
| **Audio** | microSD (SPI) + I2S amplifier — WAV files under `/audio/NNN.wav` |
| **Comms** | ESP-NOW to WALL-E base (no UART link to base) |
| **Sensors** | Dual mics, IR dock receivers, character buttons |

## Build

```bash
cd audio_esp
pio run
```

## SD card

Place 16-bit PCM WAV files on the audio node SD card, e.g. `/audio/001.wav` for track ID 1 (see `config.h`).

Shared playback engine: `lib/walle_i2s_audio/`.
