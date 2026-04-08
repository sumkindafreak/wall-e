# Web UI — LROS (Lightweight Robot Operator Shell)

Static **HTML / CSS / JavaScript** assets for operating WALL-E from a **browser**: dashboard controls, status panels, and integration with the **base** HTTP API when the page is served from the robot’s network or opened against a known base URL.

---

## Purpose

- **Secondary operator surface** without the physical CYD master controller.
- **Rapid UI prototyping** before embedding strings in firmware.
- **Documentation-friendly** — no separate flash step for the assets themselves.

---

## Files

| File | Role |
|------|------|
| [index.html](index.html) | Primary LROS page |
| [index-standalone.html](index-standalone.html) | Offline or demo variant |
| [css/lros.css](css/lros.css) | Theme variables, layout |
| [js/lros.js](js/lros.js) | Fetch helpers, UI logic, operator strip, optional vision-event toasts, `apiAuthHeaders()` for optional base token |
| [js/lros-navigation.js](js/lros-navigation.js), [js/lros-maplibre-nav.js](js/lros-maplibre-nav.js), [js/lros-sequences.js](js/lros-sequences.js), [js/navMissionPanel.js](js/navMissionPanel.js), … | Navigation, sequences, map deck |

### Embedded build (base firmware PROGMEM)

To regenerate **`main_wall_E_base/main/web_page_lros.h`** from `webui/` sources (bundled CSS + JS):

```powershell
.\webui\build-embed.ps1
```

(On Windows PowerShell from repo root; see script for exact paths.)

---

## Dependencies

- Modern browser (**ES6+**).
- **Live control:** Reachable **base** IP (e.g. `192.168.4.1` on `WALL-E-Control` AP) and routes implemented in `main_wall_E_base/main/web_server.cpp`.
- **CORS:** If opening `index.html` from `file://`, browser security may block API calls — serve from the base or use a local static server.
- **Optional API token:** If the base has a token configured (`POST /api/security/token`), set **`localStorage.walle_api_token`** in the browser to match; protected fetches send **`X-Wall-E-Token`** (see `lros.js` / `lros-sequences.js` / `lros-navigation.js`).
- **Motion policy:** Operator strip shows **`motion_policy`** (`any` / `cyd_only` / `web_only`) from **`GET /api/motion/operator`**.

---

## Deployment (no separate firmware)

| Method | Steps |
|--------|--------|
| **SPIFFS / LittleFS** | Copy `webui/` into filesystem image and upload with base firmware if your build supports it. |
| **Direct open** | Open `index.html`; configure base URL in script if required. |
| **Embedded headers** | Some builds use `web_page_*.h` — minify assets and include as C strings (project-specific). |

---

## Node ID and MAC

- **Not applicable** — browser client only.
- **Security:** Treat the base HTTP API as **trusted LAN**; optional shared token on the base for mutating routes — see [main_wall_E_base/README.md](../main_wall_E_base/README.md).

---

## Communication responsibilities

| Path | Technology |
|------|------------|
| **Browser → Base** | **HTTP** (REST or ad-hoc handlers) |
| **Master → Base** | **ESP-NOW** — parallel path; base **`motion_authority`** policy can restrict CYD vs HTTP drive |

**Chain:** LROS does **not** talk to the master CYD; both talk to the **base** (different transports). See [ARCHITECTURE.md](../ARCHITECTURE.md#2-webui--base--nodes-communication-chain).

---

## Extending the UI

1. Add markup in `index.html`.
2. Style with variables in `lros.css`.
3. Implement API calls in `lros.js`; document routes in [../main_wall_E_base/README.md](../main_wall_E_base/README.md) and [../CONTRIBUTING.md](../CONTRIBUTING.md).

---

## Known quirks and limitations

- **Auth** — Optional token on base for some POST/mutating routes; browser must send header when enabled.
- **Telemetry rate** — Browser polling interval should be reasonable to avoid overloading the base.

---

## See also

- [../CONTRIBUTING.md](../CONTRIBUTING.md)  
- [../README.md](../README.md)  
