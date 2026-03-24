# Shared Libraries (`lib/`)

Cross-project **Arduino / PlatformIO** libraries referenced via `lib_extra_dirs = ../lib` from `main_wall_E_base`, `wall_e_master_controller`, and similar.

## Contents

| Folder | Purpose |
|--------|---------|
| `walle_emotion_pose/` | Emotional pose state machine — servo-agnostic; base applies hardware. |
| `walle_audio_events/` | Audio event packet stubs — extend alongside `audio_esp`. |
| `walle_vision_events/` | Vision event stubs — extend alongside `vision_node`. |

## Dependencies

- None at the folder root; each sub-library may declare `library.json` (see `walle_emotion_pose`).

## Flashing

- Libraries are **compiled into** firmware — no separate flash step.

## Node ID / MAC

- N/A — code-only.

## Communication

- Used **by** base and controller code; not a network node.

## Calibration

- Pose limits and easing: see `walle_emotion_pose` headers and call sites in base / master.

## Related

- [../ARCHITECTURE.md](../ARCHITECTURE.md) — emotion pose overview  
- [../README.md](../README.md)  
