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
| [js/lros.js](js/lros.js) | Fetch helpers, UI logic |

---

## Dependencies

- Modern browser (**ES6+**).
- **Live control:** Reachable **base** IP (e.g. `192.168.4.1` on `WALL-E-Control` AP) and routes implemented in `main_wall_E_base/main/web_server.cpp`.
- **CORS:** If opening `index.html` from `file://`, browser security may block API calls — serve from the base or use a local static server.

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
- **Security:** Treat the base HTTP API as **trusted LAN** unless you add authentication.

---

## Communication responsibilities

| Path | Technology |
|------|------------|
| **Browser → Base** | **HTTP** (REST or ad-hoc handlers) |
| **Master → Base** | **ESP-NOW** — parallel path; avoid simultaneous conflicting drive commands |

**Chain:** LROS does **not** talk to the master CYD; both talk to the **base** (different transports). See [ARCHITECTURE.md](../ARCHITECTURE.md#2-webui--base--nodes-communication-chain).

---

## Extending the UI

1. Add markup in `index.html`.
2. Style with variables in `lros.css`.
3. Implement API calls in `lros.js`; document routes in [../main_wall_E_base/README.md](../main_wall_E_base/README.md) and [../CONTRIBUTING.md](../CONTRIBUTING.md).

---

## Known quirks and limitations

- **No built-in auth** in typical embedded demos — do not expose to the public internet without hardening.
- **Telemetry rate** — Browser polling interval should be reasonable to avoid overloading the base.

---

## See also

- [../CONTRIBUTING.md](../CONTRIBUTING.md)  
- [../README.md](../README.md)  
