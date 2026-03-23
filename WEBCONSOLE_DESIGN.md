# WALL-E Living Robot Operating System (LROS) — Web Console Design

> **Design philosophy**: A fusion of Pixar companion emotion, Tesla vehicle control, military robotics HUD, and living digital character. Users experience connection, confidence, control, and wonder.

---

## 1. System Overview

### 1.1 What LROS Is

The WALL-E Web Console is not a simple robot controller—it is a **Living Robot Operating System** that balances:

| Dimension | Goal |
|-----------|------|
| **Warmth** | Pixar-style emotional presence; WALL-E feels like a companion |
| **Safety** | Professional robotics standards; E-Stop always visible, clear control state |
| **Cinematic** | Smooth motion, HUD overlays, mission-mode fullscreen |
| **Technical** | Deep engineering access when needed; telemetry, logs, calibration |

### 1.2 Connection Architecture

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  POWER-ON FLOW                                                                │
│  1. WALL-E launches AP "WALL-E-Control" (192.168.4.1)                         │
│  2. User connects phone/tablet to AP → opens http://192.168.4.1               │
│  3. WebUI presents network scanning wizard                                    │
│  4. Credentials saved to Base NVS → propagated to Dock via ESP-NOW            │
│  5. On future boots: auto-join known network; AP fallback if fail             │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 Device Roles (Current Codebase)

| Device | Role | Web Server | UI |
|--------|------|------------|-----|
| **Base** (main_wall_E_base) | Brain: motors, IMU, autonomy, docking | ✅ port 80 | WebUI served here |
| **CYD Master Controller** | Physical touchscreen; ESP-NOW TX/RX | ❌ | Local LVGL UI |
| **Dock Station** | Charging crate; ESP-NOW beacon | ❌ | Neopixel, display |
| **Vision Node** | Optional; ESP-NOW | ❌ | — |

**Safety rule**: When CYD touchscreen is touched, Web control must disengage and show "Local Control Active" in WebUI.

---

## 2. Primary Navigation Structure

16 top-level screens with swipe transitions and fullscreen mission mode:

| Screen | Purpose | Key Features |
|--------|---------|--------------|
| **HOME** | Dashboard; emotional presence, quick status | Face, thought toasts, connection, battery |
| **DRIVE** | Manual control | Joystick, tank sliders, speed profile, E-Stop |
| **NAVIGATION** | Map, waypoints, boundaries | GPS map, patrol editor, virtual fences |
| **DOCKING** | Charge mission console | Sensor viz, approach vectors, dock status |
| **VISION** | FPV & AI vision | Live feed, snapshot, object overlay, tracking |
| **AUDIO** | Sound & voice | Soundboard, volume, personality sounds |
| **AI / PERSONALITY** | Behaviour tuning | Modes, curiosity, reward, daily routines |
| **MISSIONS** | Automation & timeline | Mission editor, simulation, conditional logic |
| **TELEMETRY** | Sensor command center | IMU viz, sonar bars, GPS, calibration |
| **POWER** | Battery & sleep | Prediction, motor profiles, sleep scheduling |
| **NETWORK** | Wi‑Fi, OTA, topology | Scan wizard, propagation, device map |
| **FILES** | Storage & backup | Browse, upload, personality export |
| **SAFETY** | E‑Stop, geo-fence, tilt | Collision tuning, child mode, watchdog |
| **LOGS** | Life history | Timeline, replay, AI journal |
| **SECURITY** | Access control | Trust pairing, permissions |
| **DEVELOPER** | Debug & raw APIs | API explorer, raw commands |

---

## 3. Emotional Robot Presence

### 3.1 Animated Face State

- **States**: Idle, Happy, Curious, Tired, Concerned, Excited, Sleeping, Docking
- **Implementation**: SVG/CSS animation or canvas; driven by `moodState` / `emotion` from `/api/autonomy`
- **Idle behaviors**: Subtle blink, slight head tilt animation, ambient "breathing" motion

### 3.2 Thought Toasts

Contextual notifications representing WALL-E's internal state:

| Emoji | Example | Trigger |
|-------|---------|---------|
| 🔋 | "I'm getting tired" | Low battery |
| 📡 | "Searching for my dock" | RTH / dock homing active |
| 😊 | "I like being with you" | Proximity / interaction |
| ⚠️ | "Something feels wrong with my sensors" | IMU/sonar anomaly |
| 🌙 | "Going to sleep" | Sleep mode entering |
| 🎯 | "Heading to waypoint" | Waypoint nav active |

- **Placement**: Bottom of screen, stack up to 3; auto-dismiss after 5–8s
- **Style**: Semi-transparent card, rounded, subtle entrance animation

### 3.3 Reactive Theme Colours

| State | Accent | Surface tint |
|-------|--------|--------------|
| Default | Warm amber `#f5a623` | Dark blue-gray |
| Low battery | Amber → red gradient | Slight red tint |
| Docking active | Green `#3ddc84` | Slight green tint |
| Error / fault | Red `#e63946` | Slight red pulse |
| Sleep / idle | Muted blue | Dimmed |

### 3.4 Emotional Memory Timeline

- Story-like event log: "Docked at 14:32", "Found something interesting near the plant"
- Filter by type: docking, navigation, AI, sensor
- Playback mode: scrub through timeline with mini-map

---

## 4. Screen Specifications

### 4.1 HOME

- Large WALL-E face (center)
- Connection badge (AP / STA / Offline)
- Battery bar + estimated runtime
- Quick actions: Drive, Docking, Network
- Thought toast area
- Swipe down → refresh status

### 4.2 DRIVE

- **Control modes**: Joystick | Tank sliders | AI-assisted
- Virtual joystick (tank mix) — existing logic in `web_page.h`
- Tank sliders: left/right -100..100
- Speed profile selector: Low / Normal / High
- GPS waypoint "Go" button
- Auto dock trigger
- **E-Stop**: Always visible, fixed position (e.g. bottom center)
- CYD override warning banner when local control active

### 4.3 NAVIGATION

- Robot-vacuum style map
- Live GPS position marker
- Saved locations (home, waypoints)
- Patrol route editor (draw waypoints)
- Virtual boundaries (no-go zones)
- Obstacle sensor overlays
- Homing / auto-dock logic toggle
- Mission timeline + simulation preview

### 4.4 DOCKING

- Sensor alignment viz (IR, ToF, beam)
- Approach vector / heading indicator
- Charge stability graph (current over time)
- Dock firmware status
- Beacon indicator (callout on/off)
- Failure recovery settings

### 4.5 VISION

- Live FPV stream (MJPEG or WebRTC if supported)
- Record / snapshot / night mode
- Object detection overlay
- Tracking mode toggle
- Face recognition status
- AI scene description text feed

### 4.6 AUDIO

- Soundboard triggers
- Volume control
- Voice personality settings
- Emotional sound behaviour
- Filesystem audio browser
- Remote listen / talkback

### 4.7 AI / PERSONALITY

- Behaviour mode: Curious / Happy / Shy / Tired / Excited
- Decision confidence slider
- Learning enable/disable
- Reward logic tuning
- Random curiosity level
- Daily routine scheduler
- Thought frequency

### 4.8 TELEMETRY

- 3D or 2D IMU orientation
- Obstacle distance bars (sonar, ToF)
- GPS satellite count / signal
- Sensor health indicators
- Calibration tools
- Failure alerts

### 4.9 POWER

- Battery prediction model
- Motor power profiles
- Sleep scheduling
- Emergency shutdown

### 4.10 NETWORK

- **Topology map**: WALL-E node, Dock, child devices, signal strength links
- OTA firmware
- Reboot tools
- Credential propagation
- Remote access settings
- Trust pairing

### 4.11 FILES

- Internal storage browser (SPIFFS/LittleFS/SD)
- Upload / delete
- Backup / restore
- Personality memory export
- AI behaviour loader

### 4.12 SAFETY

- E-Stop
- Collision tuning
- Geo-fence zones
- Child-safe mode
- Tilt protection
- Watchdog reset
- Remote control lockout

### 4.13 LOGS

- Activity timeline
- Navigation replay
- Docking history
- AI learning journal
- Sensor anomaly records
- Simulation playback

---

## 5. API Mapping (Current + Future)

### 5.1 Existing Endpoints (Base)

| Endpoint | Purpose |
|----------|---------|
| `GET /` | Main HTML |
| `GET /forward`, `/reverse`, `/left`, `/right`, `/stop` | Direction commands |
| `GET /drive?left=&right=` | Tank drive |
| `GET /speed?value=` | Speed 0–255 |
| `GET /wifi/status`, `/wifi/scan`, `/wifi/connect`, `/wifi/disconnect`, `/wifi/clear` | Wi‑Fi |
| `GET /settings`, `GET /settings/set?max_speed=` | Settings |
| `GET /api/autonomy`, `/api/autonomy/enable`, `/api/autonomy/set_home` | Autonomy |
| `GET /servo/set`, `/servo/neutral`, `/servo/status` | Servos |
| `GET /imu/status`, `/imu/recalibrate` | IMU |
| `GET /battery/status` | Battery |

### 5.2 Proposed New Endpoints

| Endpoint | Purpose |
|----------|---------|
| `GET /api/telemetry` | Unified telemetry (battery, IMU, autonomy, dock) |
| `GET /api/dock/status` | Dock state, approach stage, charge current |
| `GET /api/topology` | Device map (Base, Dock, children, RSSI) |
| `POST /api/estop` | Emergency stop |
| `GET /api/control_source` | Web vs CYD (for CYD-override banner) |
| `GET /api/vision/stream` | MJPEG or frame endpoint |
| `GET /api/logs` | Paginated log entries |
| `GET /api/files/list`, `POST /api/files/upload` | File manager |

---

## 6. UX Design Rules

- **Mobile-first**: Touch targets ≥ 44px; swipe navigation
- **Cinematic**: Panel transitions (slide/fade), subtle parallax on scroll
- **Responsive**: Phone → tablet → desktop dashboard layout
- **Theme selector**: Light / Dark / Auto
- **Haptic feedback**: `navigator.vibrate()` for critical actions (E-Stop, dock lock)
- **Voice interaction**: Optional Web Speech API for "Hey WALL-E" (future)
- **Configurable dashboard**: Drag-resizable widgets on HOME
- **Fullscreen mission mode**: Hide chrome for Navigation / Docking focus

---

## 7. Technical Stack & Deployment

### Option A: Single-file PROGMEM (Current)

- All HTML/CSS/JS in `web_page.h`
- Pros: No FS dependency, simple OTA
- Cons: Size limit (~100–200KB), hard to maintain

### Option B: SPIFFS / LittleFS

- Serve static files from flash
- Pros: Larger UI, easier iteration
- Cons: Requires FS partition, OTA includes UI assets

### Option C: Hybrid

- Core shell in PROGMEM; heavy screens (map, vision) lazy-loaded or optional
- Development: standalone HTML/JS, proxy to Base API

### Recommended

- **Phase 1**: Evolve existing single-page into LROS shell (navigation scaffold, face, toasts)
- **Phase 2**: Split heavy screens; consider LittleFS if Base has space
- **Phase 3**: Optional PWA with offline shell

---

## 8. Implementation Phases

| Phase | Focus | Deliverables |
|-------|-------|--------------|
| **1** | Design system, shell, emotional presence | New `webui/` folder; integrated into Base |
| **2** | DRIVE, HOME, NETWORK (network wizard + topology) | Full replacement of current `web_page.h` |
| **3** | DOCKING, TELEMETRY, POWER | New API endpoints, new screens |
| **4** | NAVIGATION, MISSIONS, MAP | Map lib (Leaflet/Mapbox or canvas); waypoint API |
| **5** | VISION, AUDIO, FILES | Stream, file API |
| **6** | LOGS, AI, SAFETY, SECURITY, DEVELOPER | Advanced screens |

---

## 9. Network Topology Map

```
     ┌─────────────┐
     │   WALL-E    │  ● Master node
     │   (Base)    │
     └──────┬──────┘
            │ ESP-NOW
     ┌──────┼──────┐
     │      │      │
     ▼      ▼      ▼
┌────────┐ ┌────────┐ ┌─────────┐
│  Dock  │ │  CYD   │ │ Vision  │  ● Child nodes
│        │ │ Master │ │  Node   │
└────────┘ └────────┘ └─────────┘
   RSSI      RSSI        RSSI
```

- Nodes as cards or icons
- Links with thickness/opacity = signal strength
- Colour = status (green=ok, amber=weak, red=offline)

---

## 10. Final Experience Goals

1. **Connection**: User feels WALL-E is a companion, not a gadget
2. **Confidence**: Clear feedback, E-Stop always available, no ambiguity
3. **Control**: Full technical access when needed; doesn't get in the way
4. **Wonder**: Cinematic motion, emotional toasts, sense of life
