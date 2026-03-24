# Proposed Repository Layout

**Status:** Documentation only — **no repository moves have been performed**. Use this as a migration blueprint when the team agrees to reorganize.

---

## Current layout (summary)

| Area | Folders |
|------|---------|
| Firmware | `wall_e_master_controller/`, `main_wall_E_base/`, `audio_esp/`, `vision_node/`, `vision_node_arduino/`, `dock_station/` |
| Shared code | `lib/` |
| Web | `webui/` |
| Docs / OTA | Root `README.md`, `ARCHITECTURE.md`, `OTA_README.md`, `ota_build_all.*` |
| Other | `paludarium_4relay/` (separate sketch), optional `wall-e electronic mounts/` (often untracked) |

**Pain points:** Duplicate protocol headers; two vision trees; mixed-purpose folders at repo root.

---

## Proposed layout

```
wall-e/
├── README.md
├── ARCHITECTURE.md
├── CONTRIBUTING.md
├── FOLDER_STRUCTURE.md
├── REPO_AUDIT.md
├── OTA_README.md
├── ota_config.ini
├── ota_build_all.ps1
├── ota_build_all.sh
│
├── docs/
│   ├── media/                 # Images, GIFs for README
│   └── diagrams/              # Optional exported SVGs
│
├── protocols/                 # Single source: packed structs + magic
│   ├── node_health.h
│   ├── dock.h
│   ├── vision.h
│   └── control_telemetry.h  # ControlPacket / TelemetryPacket
│
├── shared/                    # Renamed from lib/ (optional name)
│   ├── walle_emotion_pose/
│   ├── walle_audio_events/
│   └── walle_vision_events/
│
├── firmware/
│   ├── master_controller/     # wall_e_master_controller
│   ├── base/                  # main_wall_E_base
│   ├── audio/                 # audio_esp
│   ├── vision/                # Merge or pick canonical vision_node
│   ├── dock/                  # dock_station
│   └── legacy/                # vision_node_arduino if retained
│
├── webui/                     # Unchanged
├── tools/                     # Helper scripts (non-OTA)
└── third_party/
    └── paludarium_4relay/     # Optional move for clarity
```

---

## Reasoning

| Change | Benefit |
|--------|---------|
| `protocols/` | Eliminates drift; enables one CI job to compile-test includes. |
| `firmware/` | Clear “what flashes” vs docs and web assets. |
| `docs/media/` | Stable image URLs in README. |
| `third_party/` | Signals that not everything is core WALL-E firmware. |
| `shared/` (rename `lib/`) | Reduces confusion with Arduino “Library Manager” deps. |

---

## Migration plan (non-destructive)

### Phase 0 — No moves

- Add **`docs/media/`** and drop one hero image when ready.
- Document canonical **vision** path in root README (PlatformIO vs Arduino).

### Phase 1 — Include paths only

- Create **`protocols/`** with headers; add **`-I ../../protocols`** (or equivalent) to each `platformio.ini`.
- Keep old headers as **thin wrappers** `#include`ing the new file, or delete duplicates only after all builds pass.

### Phase 2 — Rename `lib/` → `shared/`

- `git mv lib shared`
- Update **`lib_extra_dirs`** and Arduino include paths in one PR.

### Phase 3 — `firmware/` renames

- `git mv wall_e_master_controller firmware/master_controller` (example)
- Update **every** `platformio.ini`, `src_dir`, and documentation link in the same PR.

### Phase 4 — Vision consolidation

- Deprecate one of `vision_node/` vs `vision_node_arduino/` or document **dual maintenance** with a checklist in `REPO_AUDIT.md`.

### Rollback

- Git history preserves paths; revert the merge commit if CI or users break.

---

## Minimal alternative

Add **`protocols/`** only — **no folder renames**. Highest ROI for correctness; lowest churn.
