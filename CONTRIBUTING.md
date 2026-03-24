# Contributing to WALL-E

Thank you for helping improve this multi-node robotics stack. This document describes workflow, commits, and how to extend nodes and the Web UI.

---

## Branching model

- **`main`** — Integration branch; should build for the primary PlatformIO environments documented in each folder’s README.
- **Feature branches** — Name with a short prefix and kebab-case:
  - `feat/` — New behavior (e.g. `feat/dock-ir-tuning`)
  - `fix/` — Bug fixes (e.g. `fix/espnow-rssi-overflow`)
  - `docs/` — Documentation only (e.g. `docs/readme-vision`)
  - `chore/` — Tooling, formatting, non-functional changes
- **Pull requests** — Open against `main` with a short description, test notes (which boards you flashed), and any protocol or pin changes called out explicitly.

Forks: keep `main` synced with upstream before large merges.

---

## Commit message style

Prefer **imperative mood** and a **scope** when helpful:

```
<type>(<scope>): <short summary>

Optional body explaining why, breaking changes, and protocol updates.
```

**Types:** `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

**Examples:**

- `feat(dock): add ir_align_hint to beacon packet`
- `fix(base): guard VL53 init when sensor absent`
- `docs: add master controller flashing steps`

If a commit touches **packed structs** or **magic bytes**, mention it in the body (e.g. `DockBeaconPacket_t` size change).

---

## How to add a new node

1. **Design the role** — Sensor-only, actuator, or hybrid; decide if it speaks ESP-NOW only, Wi-Fi HTTP, or both.
2. **Assign a node ID** — Extend `walle_node_id_t` in [node_health_protocol.h](main_wall_E_base/main/node_health_protocol.h) (and **copy the same definition** to `audio_esp/`, `dock_station/`, `wall_e_master_controller/`, etc., until a single shared header exists).
3. **Define packets** — Use packed structs, explicit magic/version fields, and document in `ARCHITECTURE.md`.
4. **Wire the base** — `main_wall_E_base` ESP-NOW receiver typically dispatches on magic + length; add a branch and a small handler module under `main/`.
5. **Wire the UI** — If the operator needs visibility, extend LROS (`webui/`) and/or master controller telemetry.
6. **README** — Add `your_node/README.md` with pins, build env, flash steps, and calibration.

Keep **Wi-Fi channel** aligned with the base AP when using ESP-NOW.

---

## How to extend the Web UI (LROS)

The LROS assets live under [`webui/`](webui/README.md).

- **Static prototype** — Open `webui/index.html` or `index-standalone.html` in a browser (some features need a live base).
- **Served from base** — The base web server may embed or serve pages; see `main_wall_E_base/main/web_server.cpp` for routes and integration points.
- **Conventions** — Keep `lros.css` variables for theming; extend `lros.js` with namespaced handlers to avoid collisions with inline scripts.
- **API contract** — When adding REST or WebSocket endpoints, document paths and JSON shapes in the base README and here if public.

---

## Protocol and config hygiene

- **Duplicate headers** — `dock_protocol.h` and `node_health_protocol.h` exist in multiple trees; if you change a struct, update **all** copies or note in the PR that a follow-up sync is required.
- **Pins** — Pin tables in READMEs are **hints**; `*_config.h` wins.

---

## Testing

- Build affected PlatformIO environments locally before submitting.
- If you cannot hardware-test, state that in the PR and list what was simulated (e.g. compile-only).

---

## Code of conduct

Be constructive and assume good intent. Hardware builds vary; document your board variant when reporting issues.
