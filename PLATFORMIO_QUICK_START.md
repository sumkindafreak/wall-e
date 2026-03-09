# PlatformIO Quick Start (WALL-E)

This repo has multiple firmware targets. Open the matching folder in VS Code when building/uploading.

## 1) Master Controller (CYD)
Folder: `wall_e_master_controller`

- Open that folder in VS Code
- Select env: `cyd_esp32_2432s028`
- Build: `PlatformIO: Build`
- Upload: `PlatformIO: Upload`

## 2) WALL-E Base Brain (ESP32-S3)
Folder: `main_wall_E_base`

- Open that folder in VS Code
- Select env: `wall_e_brain_s3`
- Build: `PlatformIO: Build`
- Upload: `PlatformIO: Upload`

## 3) Audio Brain (ESP32-S3)
Folder: `wall_e_audio`

- Open that folder in VS Code
- Select env: `wall_e_audio_s3`
- Build: `PlatformIO: Build`
- Upload: `PlatformIO: Upload`

## CLI equivalents
Run in the chosen firmware folder:

```bash
pio run
pio run -t upload
pio device list
```

## Common fixups
- If upload port is wrong, add `upload_port = COMx` to that folder's `platformio.ini`
- If monitor port is wrong, add `monitor_port = COMx`
- Close Serial Monitor before upload
- Ensure USB driver is installed (CH340 or CP210x, depending on your board)

## Note on duplicate source tree in `main_wall_E_base`
`main_wall_E_base/platformio.ini` intentionally filters sources to avoid compiling duplicate `main/main.ino` and `main/espnow_receiver.cpp` alongside top-level files.
