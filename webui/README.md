# Web UI (LROS — Lightweight Robot Operator Shell)

Static **HTML / CSS / JS** assets for operating WALL-E from a browser: dashboard-style controls, status, and hooks to the **base** HTTP API when the page is served or proxied from the robot’s network.

## Purpose

- **Operator view** without the physical CYD controller (or as a secondary panel).
- **Prototyping** UI flows before embedding in the base firmware.

## Files

| File | Role |
|------|------|
| `index.html` | Main LROS page |
| `index-standalone.html` | Offline / demo variant |
| `css/lros.css` | Theme variables, layout |
| `js/lros.js` | Control logic, fetch/API helpers |

## Dependencies

- Modern browser (ES6+).
- For **live** control: reachable **base** IP (e.g. `192.168.4.1` on WALL-E AP) and matching **API routes** implemented in `main_wall_E_base/main/web_server.cpp`.

## “Flashing” / deployment

There is **no separate firmware** — deploy by:

1. **Copy** `webui/` to SPIFFS/LittleFS if the base serves static files from flash, **or**
2. **Open** `index.html` locally and point configuration at the base URL (CORS permitting), **or**
3. **Embed** minified assets into `web_page_*.h` style headers if used by the project.

## Node ID / MAC

- N/A — browser client only. **Authentication** is project-dependent (home LAN / AP trust model).

## Communication with master controller

- **Parallel path:** Master uses **ESP-NOW**; LROS uses **HTTP** to the **base**. They are not mutually exclusive — avoid conflicting drive commands if both are active.

## Extending the UI

- Add panels in `index.html`; use **CSS variables** in `lros.css` for consistent theming.
- Add API calls in `lros.js`; document new routes in [../main_wall_E_base/README.md](../main_wall_E_base/README.md) and [../CONTRIBUTING.md](../CONTRIBUTING.md).

## Related

- [../CONTRIBUTING.md](../CONTRIBUTING.md) — WebUI extension guidelines  
- [../README.md](../README.md) — project overview  
