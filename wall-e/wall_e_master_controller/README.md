# WALL-E Master Controller — Layered UI (Oldskool)

Modular ESP32 CYD (ESP32-2432S028) WALL-E Master Controller. **TFT_eSPI + XPT2046** direct drawing — no LVGL. Layered static/dynamic rendering, proper joystick math, grid overlay, eye graphic.

## Features

- **Layered UI** — Static draw once, dynamic update only when changed
- **Grid overlay** — Subtle 20px industrial look
- **Proper virtual joystick** — Deadzone 10%, radial clamping, smooth interpolation (0.6/0.4)
- **Region-based updates** — No full-screen redraws, no flicker
- **Page system** — Drive, Behaviour, System
- **Character layer** — 24×16 eye: blink, mood-based width, E-STOP flash
- **Transmission** — Stable 50 Hz ESP-NOW
- **Safety lock** — 200 ms no touch → STOP

## Structure

| File | Purpose |
|------|---------|
| `wall_e_master_controller.ino` | Main, page state, loop |
| `ui_draw.h` / `ui_draw.cpp` | Static/dynamic rendering, regions, grid |
| `touch_input.h` / `touch_input.cpp` | Joystick math, smoothing, zones |
| `character_layer.h` / `character_layer.cpp` | Eye graphic |
| `packet_control.h` / `packet_control.cpp` | 50 Hz send, safety, telemetry |
| `protocol.h` | ControlPacket, DriveState |
| `espnow_control.h` / `espnow_control.cpp` | ESP-NOW |

## UI Regions

- `TOP_BAR_HEIGHT` 30
- `BOTTOM_BAR_Y` 200
- `DRIVE_LEFT_X` 0, `DRIVE_RIGHT_X` 160
- Joystick centers: (80, 115), (240, 115)

## Required Libraries

- TFT_eSPI — `Setup_CYD_ESP32_2432S028R`
- XPT2046_Touchscreen
- ESP32 board package
