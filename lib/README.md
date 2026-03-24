# Shared Libraries (`lib/`)

Cross-project **Arduino / PlatformIO** code pulled in via **`lib_extra_dirs = ../lib`** from `main_wall_E_base`, `wall_e_master_controller`, and other firmware folders.

---

## Contents

| Directory | Purpose |
|-----------|---------|
| [walle_emotion_pose/](walle_emotion_pose/) | Emotional pose state — **servo-agnostic**; base applies hardware. |
| [walle_audio_events/](walle_audio_events/) | Stub / shared **audio event** definitions — extend with `audio_esp`. |
| [walle_vision_events/](walle_vision_events/) | Stub / shared **vision event** definitions — extend with `vision_node`. |

---

## Dependencies

- Per-subfolder: see **`library.json`** where present (`walle_emotion_pose`).
- **Not** third-party libraries from the PlatformIO registry — those live in each project’s `lib_deps`.

---

## Build integration

- Referenced from **`platformio.ini`** via `lib_extra_dirs`.
- **No separate flash** — linked into each firmware binary.

---

## Node ID and MAC

- **N/A** — not a network node.

---

## Calibration

- **Emotion pose:** Limits and transitions in `walle_emotion_pose` headers; applied on base/master per build.

---

## Protocol note

**Dock** and **node health** headers remain in **`dock_station/`** and **`main_wall_E_base/main/`** (duplicated) — not under `lib/`. See [ARCHITECTURE.md](../ARCHITECTURE.md) and [REPO_AUDIT.md](../REPO_AUDIT.md).

---

## See also

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../README.md](../README.md)  
