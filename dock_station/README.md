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
| **Align left**   | 26  | IR sensor (left side)    |
| **Align right**  | 35  | IR sensor (right side, input-only) |
| **Arrow left**   | 19  | MOSFET → LED strip       |
| **Arrow right**  | 18  | MOSFET → LED strip       |
| **Internal LED** | 21  | MOSFET → interior light  |
| **Call switch**  | 15  | Toggle to call WALL-E (LOW = calling) |
| **OLED SDA**     | 22  | SSD1306 I2C data (when USE_OLED=1)   |
| **OLED SCL**     | 23  | SSD1306 I2C clock (often labeled SCK)|

## OLED Display (optional)

Set `USE_OLED 1` in `dock_config.h`. 0.91" SSD1306 128×32 I2C at 0x3C. Shows:
- State (NOT_DOCKED, CHARGING, FAULT, etc.)
- Current (A)
- Beam / Blocked status
- MOSFET on/off
- WiFi IP when connected

## Call WALL-E Switch

Flip the toggle to call WALL-E home. When on:
- Beacon sets `callout_active` → WALL-E starts docking
- NeoPixels show amber chase (main status)
- Arrow + internal MOSFETs run a light show (chase pattern)

## Libraries

- **FastLED** (PlatformIO lib_deps, or Library Manager → "FastLED")

## Intelligent Fault Indicators (10-pixel strip)

When in FAULT, LEDs show:

1. **Fault code** — N red blinks:
   - 1 blink = **Overcurrent** (current > 3 A)
   - 2 blinks = **Force off** (WALL-E sent DOCK_CMD_FORCE_OFF)

2. **Segment display** — sensor snapshot:
   - Pixels 0–1: **Beam** (green = broken/present, red = clear)
   - Pixels 2–3: **Mouth** (green = clear, red = blocked)
   - Pixels 4–9: **Current bar** (yellow = normal, red = overcurrent)

## Configuration

Edit `dock_config.h`:

- **`WIFI_HOME_SSID`** / **`WIFI_HOME_PASSWORD`** – Home WiFi (dock connects as permanent base). Leave SSID empty (`""`) to skip.
- `USE_OBSTACLE_SENSORS` – 1 to enable obstacles
- `ACS712_MV_PER_AMP` – 100 (20A), 185 (5A), 66 (30A)
- `NEOPIXEL_COUNT` – LED count

**Time sync**: Dock gets NTP when on WiFi. WALL-E also shares time via ESP-NOW every 60 s when connected (fallback if dock can't reach NTP). Set `TIMEZONE_OFFSET_SEC` in `dock_config.h` for local time.

**WALL-E home WiFi (single config)**: Configure WiFi on WALL-E only: connect to AP `WALL-E-Control`, open 192.168.4.1 → Settings → WiFi, enter home network and connect. Then tap **Share WiFi with dock** — credentials are sent via ESP-NOW. The dock stores them in NVS and connects. Both will be on the same LAN. No need to edit `dock_config.h` if using Share.

## Approach Mode (Arrow Staging)

| Distance   | Stage   | Arrows |
|------------|---------|--------|
| >1 m       | FAR     | Off (ESP-NOW homing) |
| 200 mm–1 m | 1 m     | On, guiding |
| 60–200 mm  | 20 cm   | Precision (faster blink) |
| Beam break | Docked  | Both solid |

WALL-E sends approach stage via ESP-NOW; dock falls back to align sensors after 2 s timeout.

## States

- **BOOT** → **NOT_DOCKED** / **DOCKED_IDLE**
- **NOT_DOCKED** – Beam not broken, charge off
- **DOCKED_IDLE** – Beam broken, debounce 1.5 s, mouth must be clear
- **CHARGING** – MOSFET on, current above 0.2 A
- **CHARGED** – Current below 0.06 A for 90 s, MOSFET off
- **FAULT** – Overcurrent or invalid state, MOSFET off

## ESP-NOW Beacon

Sent at 10 Hz to broadcast MAC. Packet: `dock_id`, `uptime_ms`, `state`, `beam_present`, `mouth_blocked`, `charge_enabled`, `current_a_x100`.
