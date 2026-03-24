# Shared WALL-E libraries

Arduino / PlatformIO: add `lib_extra_dirs = ../lib` (from `main_wall_E_base` or `wall_e_master_controller`) or copy headers into sketch folders for Arduino IDE-only builds.

| Folder | Purpose |
|--------|---------|
| `walle_emotion_pose` | Servo-agnostic emotional pose state + trigger inputs |
| `walle_audio_events` | Audio event packet stub (extend with audio_esp) |
| `walle_vision_events` | Vision event packet stub (extend with vision_esp) |

Dock protocol remains in `dock_station/dock_protocol.h` and `main_wall_E_base/main/dock_protocol.h` (keep copies in sync).
