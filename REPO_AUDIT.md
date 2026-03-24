# Repository Audit (Documentation & Structure)

**Scope:** Documentation gaps, naming consistency, protocol coverage, comment opportunities, and **future** refactors. **No source code was modified** to produce this report.

**Date:** Generated as part of documentation refinement.

---

## 1. Missing documentation

| Gap | Recommendation |
|-----|------------------|
| **Single “channel sync” guide** | Add `docs/RF_ESP_NOW.md`: set router/AP channel = base AP channel; verify with `WiFi.channel()`. |
| **`docs/media/`** | Root README references placeholders; create folder + one real image when available. |
| **LICENSE** | Root README notes absence; add SPDX file (MIT, Apache-2.0, etc.). |
| **CHANGELOG.md** | Optional; helps hardware builders track breaking protocol changes. |
| **Vision canonical path** | Explicitly state whether `vision_node/` (PlatformIO) or `vision_node_arduino/` is primary for new work. |
| **Audio protocol** | `audio_protocol.h` scattered — central summary table in ARCHITECTURE.md when stable. |
| **Master ↔ base packet version field** | Control/Telemetry have no explicit version byte — document upgrade strategy if structs evolve. |

---

## 2. Inconsistent naming

| Observation | Note |
|-------------|------|
| **`main_wall_E_base` vs “base”** | Folder name is historical; docs use “base” — acceptable if README states mapping once. |
| **`wall_e_audio` include path** | Referenced by `audio_esp` and others; not always a top-level folder in listing — verify on clone. |
| **`node_health_protocol.h` copies** | Same filename in `dock_station/`, `audio_esp/`, `wall_e_master_controller/`, etc. — risk of editing wrong file. |
| **`dock_protocol.h` copies** | Same — prefer grep across repo before editing. |
| **LROS vs webui** | “LROS” is the product name; folder is `webui/` — documented in README. |

---

## 3. Undocumented or under-documented protocols

| Protocol | Location | Gap |
|----------|----------|-----|
| **WalleNodeHealthPacket_t** | `node_health_protocol.h` | Timing guarantees not strict — document “best effort” and consumer timeouts. |
| **DockBeaconPacket_t** | `dock_protocol.h` | Field-by-field table in ARCHITECTURE (partially done). |
| **Audio ESP-NOW** | `audio_esp`, `audio_protocol.h` | End-to-end flow vs base not fully narrated in one place. |
| **Node health from dock** | `dock_espnow.cpp` | How dock populates health fields vs beacon — worth a short comment block in ARCHITECTURE. |
| **CYD ControlPacket** | `protocol.h` | Servo array length and behaviour modes — table in master README. |

---

## 4. Areas needing comments (source — suggestions only)

| Area | Suggestion |
|------|------------|
| **ESP-NOW dispatch** | In `espnow_receiver.cpp`, a short comment block listing **magic → handler** improves onboarding. |
| **`build_src_filter` exclusions** | `walle_emotion_pose.cpp` excluded in base `platformio.ini` — one-line comment why duplicate symbol is avoided. |
| **Dock `ENABLE_WIFI 0`** | When Wi-Fi off, `ir_align_hint` not sent over air — comment near beacon fill. |
| **IR alignment vs beam** | Already separated in code; add file-level note in `dock_sensors.cpp` if still confusing. |

*Do not edit code unless the team approves; this is a checklist.*

---

## 5. Refactoring opportunities (do not implement without ticket)

| Idea | Benefit | Risk |
|------|---------|------|
| **Single `protocols/` tree** | One truth for structs | Touches all PlatformIO projects |
| **Generated headers** | Script emits C headers from one JSON/YAML | Tooling maintenance |
| **Unify vision projects** | One build path | May drop Arduino IDE users |
| **Extract `espnow_dispatch.cpp` on base** | Smaller `onRecv` | Refactor churn |
| **CI: `pio run` matrix** | Catch breakages | Runner cost |

---

## 6. Security / operations notes (documentation)

| Topic | Note |
|-------|------|
| **AP default password** | Document whether `WALL-E-Control` uses open or WPA and rotate for public demos. |
| **HTTP API on base** | No auth in many embedded demos — note “trusted LAN only”. |
| **OTA** | Protect update URL on untrusted networks. |

---

## 7. Summary priority list

1. **LICENSE + optional CHANGELOG** — Legal clarity and upgrade path.  
2. **`protocols/` consolidation** — Reduces real bugs.  
3. **RF channel guide** — Reduces “ESP-NOW silent” support issues.  
4. **Vision canonical README** — Reduces wrong sketch flashed.  
5. **CI smoke build** — Catches struct skew early.  

---

## Related

- [FOLDER_STRUCTURE.md](FOLDER_STRUCTURE.md) — proposed layout  
- [CONTRIBUTING.md](CONTRIBUTING.md) — how to change things safely  
- [ARCHITECTURE.md](ARCHITECTURE.md) — system design  
