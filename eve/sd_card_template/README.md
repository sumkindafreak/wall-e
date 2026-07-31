# EVE SD card layout

Copy the contents of this folder to the root of EVE's microSD card.

```
/config/settings.json   — runtime toggles (optional)
/config/robot.json      — personality weights
/config/audio.json      — track index → WAV filename map
/config/display.json    — face/display preferences (optional)
/audio/*.wav            — 16-bit PCM WAV files
/images/                — future LVGL assets
/animations/            — future expression clips
/logs/                  — optional runtime logs
/updates/               — future OTA packages
```

Audio path on device: `/audio/NNN.wav` when not listed in `audio.json`.
