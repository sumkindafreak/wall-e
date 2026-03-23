# WALL-E Audio / Voice / Dock Sensor Brain

Dedicated ESP32-S3 firmware for the Audio ESP module. This board handles audio playback, microphones, voice commands, and IR dock receivers. It communicates with the main Base ESP32-S3 via **ESP-NOW**.

**Rule:** This board never controls drive motors. It only senses, interprets, plays sound, and reports.

## Quick Start

### PlatformIO

```bash
cd audio_esp
pio run
pio run -t upload
```

### Arduino IDE

1. Install **DFRobotDFPlayerMini** via Library Manager (Sketch → Include Library → Manage Libraries).
2. Open `audio_esp/audio_esp.ino`.
3. Select board: **ESP32S3 Dev Module** (or your board).
4. Ensure `wall_e_audio` folder (sibling to `audio_esp`) contains `audio_protocol.h`.
5. Verify and upload.

## Pin Configuration

Edit `pins.h` with your final GPIO assignments. Defaults:

| Function        | Pin |
|-----------------|-----|
| DFPlayer TX     | 17  |
| DFPlayer RX     | 18  |
| Left Mic (ADC)  | 4   |
| Right Mic (ADC) | 5   |
| IR Dock Left    | 6   |
| IR Dock Right   | 7   |
| Status LED      | 8   |

**Base link:** ESP-NOW on WiFi channel 11 (no UART pins).

## Protocol

- **Incoming:** `WalleAudioCommandPacket_t` (play track, volume, stop) from Base.
- **Outgoing:** `WalleAudioMicTelemPacket_t` (ear levels), `WalleAudioStatusPacket_t` (mic dir, dock IR, mode, fault).

See `ARCHITECTURE.md` and `wall_e_audio/audio_protocol.h` for details.

## SD Card (DFPlayer)

Place MP3 files on the SD card as `001.mp3`, `002.mp3`, etc. Track IDs are defined in `config.h` (e.g. TRACK_STARTUP=1, TRACK_HELLO=2, ...).
