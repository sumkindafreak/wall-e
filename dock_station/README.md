# Smart Charging Crate v1.1

WALL-E docking station — **permanent connectivity base**. Connects to home WiFi, broadcasts ESP-NOW beacon for homing, detects dock presence via IR beam, controls charging via MOSFET, and drives NeoPixel status LEDs. WALL-E's home = the dock location.

## Hardware

| Component        | Pin | Notes                    |
|------------------|-----|--------------------------|
| MOSFET gate      | 25  | Charge enable            |
| ACS712 ADC       | 34  | Current sense (ADC1)     |
| IR beam sensor   | 27  | Beam broken = present    |
| Obstacle 1–4     | 14, 13, 33, 32 | Optional  |
| NeoPixel data    | 2   | WS2812/WS2812B           |

## Libraries

- **Adafruit NeoPixel** (Sketch → Include Library → Manage Libraries → "NeoPixel")

## Configuration

Edit `dock_config.h`:

- **`WIFI_HOME_SSID`** / **`WIFI_HOME_PASSWORD`** – Home WiFi (dock connects as permanent base). Leave SSID empty (`""`) to skip.
- `USE_OBSTACLE_SENSORS` – 1 to enable obstacles
- `ACS712_MV_PER_AMP` – 100 (20A), 185 (5A), 66 (30A)
- `NEOPIXEL_COUNT` – LED count

**Time sync**: Dock gets NTP when on WiFi. WALL-E also shares time via ESP-NOW every 60 s when connected (fallback if dock can't reach NTP). Set `TIMEZONE_OFFSET_SEC` in `dock_config.h` for local time.

**WALL-E home WiFi (single config)**: Configure WiFi on WALL-E only: connect to AP `WALL-E-Control`, open 192.168.4.1 → Settings → WiFi, enter home network and connect. Then tap **Share WiFi with dock** — credentials are sent via ESP-NOW. The dock stores them in NVS and connects. Both will be on the same LAN. No need to edit `dock_config.h` if using Share.

## States

- **BOOT** → **NOT_DOCKED** / **DOCKED_IDLE**
- **NOT_DOCKED** – Beam not broken, charge off
- **DOCKED_IDLE** – Beam broken, debounce 1.5 s, mouth must be clear
- **CHARGING** – MOSFET on, current above 0.2 A
- **CHARGED** – Current below 0.06 A for 90 s, MOSFET off
- **FAULT** – Overcurrent or invalid state, MOSFET off

## ESP-NOW Beacon

Sent at 10 Hz to broadcast MAC. Packet: `dock_id`, `uptime_ms`, `state`, `beam_present`, `mouth_blocked`, `charge_enabled`, `current_a_x100`.
