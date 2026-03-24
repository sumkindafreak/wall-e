# Proposed Repository Layout (Suggestion Only)

**Status:** Documentation-only — **no files have been moved** in the repository as part of this proposal. This is a roadmap for maintainability.

---

## Current pain points

- **Duplicate protocol headers** (`dock_protocol.h`, `node_health_protocol.h`) across `dock_station/`, `main_wall_E_base/main/`, `audio_esp/`, `wall_e_master_controller/`, etc.
- **Two vision entry points** — `vision_node/` (PlatformIO) and `vision_node_arduino/` (Arduino-style) confuse newcomers.
- **Web UI** (`webui/`) is separate from base `web_server` assets — good for static dev, but paths are easy to duplicate.
- **Misc projects** (`paludarium_4relay/`) at repo root alongside WALL-E — scope creep in one clone.

---

## Suggested top-level layout

```
wall-e/
├── README.md
├── ARCHITECTURE.md
├── CONTRIBUTING.md
├── FOLDER_STRUCTURE.md
├── OTA_README.md
├── ota_config.ini
├── ota_build_all.ps1
├── ota_build_all.sh
│
├── firmware/                      # All flashable ESP32 projects
│   ├── master_controller/         # was: wall_e_master_controller
│   ├── base/                      # was: main_wall_E_base
│   ├── audio/                     # was: audio_esp
│   ├── vision/                    # merge vision_node + vision_node_arduino OR pick one canonical
│   ├── dock/                      # was: dock_station
│   └── legacy/                    # optional: vision_node_arduino if kept for reference
│
├── protocols/                     # SINGLE source of truth (packed structs + magic)
│   ├── node_health.h
│   ├── dock.h
│   ├── vision.h
│   └── control.h                  # ControlPacket / TelemetryPacket
│
├── shared/                        # was: lib/
│   └── walle_emotion_pose/
│
├── webui/                         # LROS static assets (unchanged)
│
├── docs/                          # Images, PDFs, diagrams
│   └── media/
│
├── tools/                         # Scripts not tied to OTA (name shortening, etc.)
│
└── third_party/                   # optional: paludarium_4relay or non–WALL-E sketches
```

---

## Justification

| Change | Why |
|--------|-----|
| `firmware/*` | One place to look for PlatformIO/Arduino projects; shorter paths in docs. |
| `protocols/` | Eliminates drift between copies; CI can check one header. |
| `shared/` | Clearer than `lib/` for Arduino users who expect “library” = third party. |
| `docs/media/` | Keeps README image links stable. |
| `third_party/` | Isolates non–WALL-E experiments from the main narrative. |

---

## Migration strategy (when you choose to implement)

1. Add `protocols/` and **symlink or include path** first (build flags `-I ../protocols`) — **no behavior change**.
2. Delete duplicate headers **only after** all projects compile against the single tree.
3. Rename folders in a **dedicated PR** with `git mv` to preserve history.
4. Update `lib_extra_dirs` / Arduino IDE include paths.

---

## What not to do

- Do not move folders without updating **every** `platformio.ini` and Arduino IDE sketch path.
- Do not delete `vision_node_arduino/` until its features are merged or explicitly deprecated in README.

---

## Alternative (minimal)

If you prefer zero moves: add only **`protocols/`** as the single source of truth and keep existing folder names. That alone reduces the highest-risk drift.
