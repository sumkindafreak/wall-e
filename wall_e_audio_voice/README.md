# WALL-E Audio with Voice Commands

**Wake word:** "Hi Wall-E" → plays track 2, then listens for commands for ~12 seconds.

**28 voice commands** – volume control + 24 phrases that trigger audio replies or actions. See [VOICE_COMMANDS.md](VOICE_COMMANDS.md) for the full list.

**Build with PlatformIO only** – Arduino IDE cannot build this (requires ESP-IDF + ESP-SR).

```bash
cd wall_e_audio_voice
pio run
pio run -t upload
```

Requires ESP32-S3 with 8MB PSRAM. Same hardware as `wall_e_audio` (DFPlayer + INMP441).
