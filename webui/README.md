# WALL-E LROS Web Console

Full Living Robot Operating System web interface — 16 screens, emotional presence, network topology, and more.

## Quick Start

### 1. Local development

Serve the `webui/` folder and open in a browser:

```bash
cd webui
python -m http.server 8080
# Open http://localhost:8080
```

For live WALL-E data, either:
- Connect your machine to WALL-E's AP and open `http://192.168.4.1` (if Base serves this UI), or
- Use a proxy: set `BASE = 'http://192.168.4.1'` in `js/lros.js` and enable CORS.

### 2. Standalone (single file, no external deps)

Use `index-standalone.html` — CSS and JS are inlined. Works when:
- Opened directly from disk
- Served from any HTTP server
- Embedded in ESP32 PROGMEM (see below)

### 3. Deploy to WALL-E Base (ESP32)

**Option A: Replace embedded page**

1. Run `build-embed.ps1` to generate `web_page_lros.h`
2. In `main_wall_E_base/main/web_page.h`, replace the `WALLE_PAGE` content with the content from `web_page_lros.h`, or
3. Include `web_page_lros.h` and change `handleRoot()` to use `WALLE_PAGE_LROS` instead of `WALLE_PAGE`

**Option B: LittleFS (future)**

Serve `webui/` from SPIFFS/LittleFS for easier OTA updates.

## Screens

| Screen      | Features                                              |
|------------|--------------------------------------------------------|
| HOME       | Face, battery, quick actions, thought toasts           |
| DRIVE      | Joystick, tank sliders, speed profile, Go to Dock      |
| DOCKING    | Approach viz, charge status                            |
| NETWORK    | Topology map, Wi-Fi wizard                             |
| NAVIGATION | Map, waypoints (via More)                              |
| VISION     | FPV placeholder, snapshot (via More)                   |
| AUDIO      | Soundboard, volume (via More)                          |
| AI         | Behaviour mode, personality sliders (via More)         |
| MISSIONS   | Patrol, RTH (via More)                                 |
| TELEMETRY  | IMU, sonar, GPS (via More)                             |
| POWER      | Battery, sleep (via More)                              |
| FILES      | Storage browser (via More)                             |
| SAFETY     | E-Stop, child mode (via More)                          |
| LOGS       | Activity timeline (via More)                           |
| SECURITY   | Trust pairing (via More)                               |
| DEVELOPER  | API explorer (via More)                                |

## API Compatibility

The UI calls these existing Base endpoints:

- `/drive`, `/stop`, `/speed` — drive control
- `/wifi/status`, `/wifi/scan`, `/wifi/connect`, `/wifi/disconnect`, `/wifi/clear`
- `/api/autonomy`, `/api/autonomy/enable`, `/api/autonomy/set_home`
- `/imu/status`, `/imu/recalibrate`
- `/battery/status`
- `/settings`, `/settings/set`

Placeholder calls (no backend yet):

- `/api/vision/stream`, `/api/audio/play`, `/api/files/list`, `/api/dock/cancel`, `/api/personality/mode`, `/api/sleep`

## Design

See [WEBCONSOLE_DESIGN.md](../WEBCONSOLE_DESIGN.md) for the full specification.
