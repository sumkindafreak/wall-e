# WALL-E OTA (Over-the-Air) Update System

All WiFi-enabled WALL-E devices support OTA firmware updates.

## Devices & Methods

| Device | Method | Access |
|--------|--------|--------|
| **Base** (main_wall_E_base) | ArduinoOTA + Web | `http://192.168.4.1/update` or port 3232 |
| **Controller** (wall_e_master_controller) | ArduinoOTA | Port 3232 (connects to WALL-E AP) |
| **Dock** (dock_station) | ArduinoOTA | Port 3232 (on home WiFi) |

## Quick Start

### 1. Enable OTA in platformio.ini

For each project, uncomment and set `upload_protocol` and `upload_port`:

**Base** (`main_wall_E_base/platformio.ini`):
```ini
upload_protocol = espota
upload_port = 192.168.4.1    ; AP IP, or your base's STA IP
```

**Controller** (`wall_e_master_controller/platformio.ini`):
```ini
upload_protocol = espota
upload_port = 192.168.4.2    ; Controller gets 192.168.4.x when on WALL-E AP
```

**Dock** (`dock_station/platformio.ini`):
```ini
upload_protocol = espota
upload_port = 192.168.1.xxx  ; Dock's IP on home WiFi (check router/DHCP)
```

### 2. Upload via PlatformIO

```bash
# Single device
cd main_wall_E_base
pio run -t upload

# All devices (from repo root)
./ota_build_all.ps1 -Upload     # Windows
./ota_build_all.sh upload       # Linux/macOS
```

### 3. Web Update (Base only)

1. Build base: `cd main_wall_E_base && pio run`
2. Open `http://192.168.4.1/update` in a browser (connect to WALL-E-Control AP)
3. Select the `.bin` from `.pio/build/wall_e_brain_s3/firmware.bin`
4. Click Update

## Network Setup

- **Base**: Always has AP `WALL-E-Control` (192.168.4.1). Optionally connects to home WiFi.
- **Controller**: Connects to WALL-E-Control AP for OTA (gets 192.168.4.2).
- **Dock**: Connects to home WiFi (credentials from WALL-E via ESP-NOW or `dock_config.h`).

## Hostnames (mDNS)

- `wall-e-base.local` — Base
- `wall-e-controller.local` — Controller  
- `wall-e-dock.local` — Dock

Use these instead of IPs if mDNS works on your network.
