# WALL-E Voice Commands

Voice wake-up: say **"Hi Wall-E"** → plays track **2** (002.mp3).

The voice firmware is in a **separate project** because it requires ESP-IDF and ESP-SR, which Arduino IDE cannot build:

**Use the `wall_e_audio_voice/` folder** and build with PlatformIO:

```bash
cd wall_e_audio_voice
pio run
pio run -t upload
```

Requires ESP32-S3 with 8MB PSRAM. Same hardware as `wall_e_audio`.
