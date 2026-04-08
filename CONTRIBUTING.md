# Contributing to WALL-E

Thank you for contributing to this multi-node robotics stack. This guide covers branching, commits, **coding and documentation conventions**, adding nodes, and extending the Web UI.

---

## Table of contents

1. [Branching model](#branching-model)
2. [Commit messages](#commit-message-style)
3. [Coding conventions](#coding-conventions)
4. [Documentation conventions](#documentation-conventions)
5. [Adding a new node](#how-to-add-a-new-node)
6. [Extending the Web UI (LROS)](#how-to-extend-the-web-ui-lros)
7. [Protocol hygiene](#protocol-and-config-hygiene)
8. [Testing and review](#testing-and-review)
9. [Community](#community-standards)

---

## Branching model

| Branch | Purpose |
|--------|---------|
| **`main`** | Integration branch; should build for the primary PlatformIO environments listed in each module README. |
| **`feat/*`** | New features, e.g. `feat/dock-ir-tuning` |
| **`fix/*`** | Bug fixes, e.g. `fix/espnow-rssi` |
| **`docs/*`** | Documentation only |
| **`chore/*`** | Tooling, formatting, non-functional changes |

**Pull requests:** Target `main`. Include: what changed, which boards you flashed, and any **protocol size / magic byte** changes.

Forks should sync `main` from upstream before large merges.

---

## Commit message style

Use **imperative** mood and a **scope** when it helps:

```
<type>(<scope>): <short summary>

Body: why, breaking changes, protocol updates, migration notes.
```

**Types:** `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

**Examples**

- `feat(dock): add ir_align_hint to DockBeaconPacket_t`
- `fix(base): skip VL53 init when probe fails`
- `docs(master): document CYD env name`

If you change **packed structs**, **magic values**, or **on-wire layout**, state it explicitly in the body (consumers include other ESP32 nodes and possibly Python tools).

---

## Coding conventions

| Topic | Guideline |
|-------|-----------|
| **Language** | C++ (Arduino / ESP-IDF style); avoid non-portable extensions unless already used in file. |
| **Structs on wire** | `__attribute__((packed))` or `#pragma pack` consistently with existing file; document endianness if non-trivial. |
| **Magic numbers** | Prefer `#define` or `constexpr` with descriptive names; match existing style in the file. |
| **Pins** | Centralize in `*_config.h`; avoid magic GPIO numbers in random `.cpp` files. |
| **Serial** | Rate-limit or gate verbose logs behind a `#define DEBUG` or similar to avoid flooding. |
| **ESP-NOW** | Check `len >= sizeof(Packet)` before casting; validate `magic` before dispatch. |
| **Naming** | Follow surrounding code: `snake_case` vs `camelCase` as per existing module. |

---

## Documentation conventions

| Topic | Guideline |
|-------|-----------|
| **Module README** | Every flashable firmware folder should have a `README.md` with purpose, env name, flash steps, and pointer to authoritative `*_config.h` for pins. |
| **Pin tables** | README tables are **hints**; `*_config.h` is always authoritative. Add a one-line disclaimer in tables. |
| **Cross-links** | Link to `ARCHITECTURE.md` for protocol narrative; link to `OTA_README.md` for OTA. |
| **Diagrams** | Mermaid is supported on GitHub; keep ASCII fallbacks for offline readers where useful. |
| **Changelog** | For user-visible behavior, a short note in the PR description is enough unless you maintain `CHANGELOG.md`. |
| **Base HTTP / LROS** | When you add or change `web_server.cpp` routes, update **[main_wall_E_base/README.md](main_wall_E_base/README.md)** (and run **`webui/build-embed.ps1`** if the embedded `web_page_lros.h` should match `webui/`). |

---

## How to add a new node

1. **Define the role** — Sensor-only, actuator, or mixed; ESP-NOW only vs HTTP vs both.
2. **Reserve a node ID** — Extend `walle_node_id_t` in [node_health_protocol.h](main_wall_E_base/main/node_health_protocol.h) and **update every duplicate copy** in the repo (or add a shared include first — see [REPO_AUDIT.md](REPO_AUDIT.md)).
3. **Define packets** — Magic + version + packed struct; document in `ARCHITECTURE.md`.
4. **Wire the base** — Add dispatch in `espnow_receiver.cpp` (or equivalent) with length checks.
5. **Wire UI** — If operators need visibility, extend LROS and/or master telemetry.
6. **Add `README.md`** — Build env, pins, quirks, channel requirements.

---

## How to extend the Web UI (LROS)

- **Assets:** [`webui/`](webui/README.md) — `index.html`, `css/lros.css`, `js/lros.js`.
- **Integration:** Base routes live in `main_wall_E_base/main/web_server.cpp`.
- **Styling:** Prefer CSS variables in `lros.css` for colors and spacing.
- **JS:** Namespace or prefix handlers to avoid global collisions.
- **API contract:** Document new paths and JSON shapes in the base README and in PR description.

---

## Protocol and config hygiene

- **Duplicate headers** — `dock_protocol.h` and `node_health_protocol.h` are copied in multiple trees. A single edit must either touch **all** copies or be split into a “sync follow-up” PR (call that out explicitly).
- **Pins** — Never edit README-only pin tables without updating `*_config.h` or you will confuse builders.

---

## Testing and review

- Run **`pio run -e <env>`** for every environment you touch.
- State **hardware vs compile-only** testing in the PR.
- For RF changes, note **channel** and **board model**.

---

## Community standards

Assume good intent; hardware varies. When reporting issues, include **board**, **ESP32 Arduino core version**, and **steps to reproduce**.
