# Arduino IDE — WALL-E Base (recommended workflow)

Use **Arduino IDE only** for this repo unless you need PlatformIO for CI or advanced tooling. You can ignore `platformio.ini` for normal upload and edit cycles.

---

## Open the correct sketch (important)

Arduino requires: **sketch folder name = main `.ino` name** (without `.ino`).

| Path | Valid? | Notes |
|------|--------|--------|
| `main_wall_E_base/main/` + `main.ino` | **Yes** | **Use this.** Folder `main` matches `main.ino`. |
| `main_wall_E_base/main.ino` at repo root | **No** | Folder is `main_wall_E_base` but file is `main.ino` — names do not match; IDE may error or behave oddly. |

**What to do:** **File → Open** → select the folder  
`…/wall-e/main_wall_E_base/main`  
then open `main.ino`.

All `*.cpp` / `*.h` in that same `main/` folder are built with the sketch. Protocol headers that must live next to the sources (so Arduino finds them without extra include paths) are kept there, e.g. `audio_protocol.h`, `node_health_protocol.h` — keep them in sync with `wall_e_audio/` when you change protocols.

There is also a **root** `main_wall_E_base/main.ino` used by PlatformIO layouts; for Arduino day-to-day, treat **`main/main.ino`** as your real sketch.

---

## Board and upload

1. **Tools → Board** → ESP32 Arduino → **ESP32S3 Dev Module** (or your exact module).
2. **Tools → Port** → the Base COM port.
3. **Sketch → Upload**
4. **Tools → Serial Monitor** → **115200** baud.

---

## Libraries (Library Manager)

Install as needed for your build:

- **VL53L1X** (Pololu) — docking / ToF, if you use it  
- Others per your `*.cpp` includes (GFX, ST7789, TinyGPSPlus, MPU6050, etc.)

---

## Docking / hardware notes

- Rear obstacle pins: GPIO **20** and **47** (not 33/34 — PSRAM on many S3 boards).

---

## Boot loop `TG1WDT_SYS_RST`

1. Sketch already uses `delay(1)` / `yield()` in `loop` where applicable.  
2. If it persists: **Tools → Loop Task Stack Size** → **16384** if your core exposes it.

---

## Other WALL-E firmware folders (Arduino)

Same rule: **folder name = `.ino` name**.

| Project folder | Open this folder in Arduino | Main sketch file |
|----------------|----------------------------|------------------|
| `audio_esp/` | `audio_esp` | `audio_esp.ino` |
| `dock_station/` | `dock_station` | `dock_station.ino` |
| `wall_e_master_controller/` | `wall_e_master_controller` | `wall_e_master_controller.ino` |

Local copies of shared headers (e.g. `audio_esp/node_health_protocol.h`) exist so Arduino does not rely on `../wall_e_audio` include paths.

---

## PlatformIO

`platformio.ini` is optional for you. Developers using PlatformIO use it for unified builds; it does not affect Arduino IDE. You do **not** need to install or learn PlatformIO to work on this project.
