# Phase J — EVE LVGL 9 eye / face system

## 1. What was replaced (EVE-only legacy face code)

| Location | Before | After |
|----------|--------|--------|
| `src/eyes_control.cpp`, `arduino_ide/.../eyes_control.cpp` | Stubs: `eyesInit` / `eyesTick` / `eyesSetMode` did nothing useful when `EVE_ENABLE_EYES` was 0; no LVGL, no drawing. | Thin façade: forwards to `eveFaceDisplayInit` / `eveFaceDisplayTick`, `eveExpressionSetLegacyMode`, and notify APIs. |
| `include/config.h` | TFT pins defined but unused by eyes. | Adds `EVE_FACE_LCD_*`, `EVE_FACE_LVGL_BUF_LINES`, `EVE_FACE_DEBUG_BENCH`, `EVE_FACE_GFX_READY`. |
| N/A | No prior hand-drawn TFT loop in-tree. | **New** modular LVGL 9 subsystem under `eve_expression_state`, `eve_eye_renderer`, `eve_eye_animations`, `eve_face_display`. |

WALL-E base display code was **not** modified.

## 2. New file layout (PlatformIO `eve/`)

| File | Role |
|------|------|
| `include/lv_conf.h` | LVGL 9 configuration (RGB565, fonts, draw SW, logging). |
| `include/eve_expression_state.h` | Expression enum, `EveEyeTarget`, state-machine inputs + queries. |
| `src/eve_expression_state.cpp` | Maps battery, UART/WALL‑E connection, docking, attachment, legacy mode, bench keys → expression + targets. |
| `include/eve_eye_renderer.h` | `EveEyeUi` (LVGL object handles); init + apply merged layout. |
| `src/eve_eye_renderer.cpp` | Builds layered LVGL scene: root, visor, scan bar, glow, eyes, lids; applies geometry/opacity/tilt. |
| `include/eve_eye_animations.h` | Blink, double blink, micro-saccades, scan-line motion, wake animation hooks. |
| `src/eve_eye_animations.cpp` | Non-blocking blink FSM, idle saccade noise, scan bar drift. |
| `include/eve_face_display.h` | Public init/tick for the face stack. |
| `src/eve_face_display.cpp` | `lv_init`, display + partial buffer, flush to **Arduino_GFX** (ST7789) when `EVE_FACE_GFX_READY`, else **stub flush**; merges expression + animation; `lv_timer_handler`. |
| `include/eyes_control.h` | Adds `eyesNotifyWallEConnected`, `eyesNotifyWallEDisconnected`, `eyesNotifyDockingState`, `eyesNotifyRecordFailure`, `eyesNotifySharedVoicebox`. |

Arduino IDE: same headers/sources copied into `arduino_ide/EVE_Companion/` (plus `lv_conf.h` next to the sketch). Install **LVGL 9.x** and **GFX Library for Arduino** from Library Manager.

## 3. Hardware / build assumptions

- **Target:** ESP32-S3 (Arduino framework); **color:** RGB565.
- **Resolution:** `EVE_FACE_LCD_HOR_RES` × `EVE_FACE_LCD_VER_RES` (default **240×280**). Match your panel; ST7789 is often 240×320 — adjust offsets in the ST7789 driver if you see banding.
- **Pins:** `EVE_TFT_SPI_*`, `EVE_TFT_LEFT_CS`, `EVE_TFT_DC`, `EVE_TFT_RST` in `config.h`. When any required pin is `< 0`, `EVE_FACE_GFX_READY` is false: LVGL still runs but the flush callback does not push pixels (useful for CI / bring-up).
- **RAM:** Partial buffer = `HOR_RES * EVE_FACE_LVGL_BUF_LINES * 2` bytes (+ LVGL `LV_MEM_SIZE` in `lv_conf.h`). Prefer PSRAM-enabled boards for headroom.
- **Dependencies (`platformio.ini`):** `lvgl/lvgl @ ^9.2.0`, `moononournation/GFX Library for Arduino`, `LV_CONF_INCLUDE_SIMPLE`, `-I include`.

Enable the stack: `#define EVE_ENABLE_EYES 1` in `config.h` after wiring the panel.

## 4. How each expression is rendered (summary)

All expressions share the **same** widget stack; `EveEyeTarget` drives numeric style:

- **Visor:** Near-black rounded panel (`#060608`), optional **decidegree tilt** via transform.
- **Eyes:** Two blue rounded shapes (`#3a7cff`) with **shadow** for soft depth; **inter-eye gap** from `eyeSep`; **scale X/Y** for widen/squash/happy.
- **Glow:** Larger, semi-transparent blue panels behind each eye; opacity follows `glowOpa` (+ subtle sine **breathing** in `eve_face_display`).
- **Lids:** Black bars on top of each eye; height = `lid` (0–1) merged with **blink** animation.
- **Scanline:** Thin horizontal bar; vertical drift + `scanOpa` (higher for CURIOUS / CONFUSED / ALERT / SEARCHING).
- **Gaze:** Translates both eyes horizontally/vertically from `gazeX` / `gazeY` (−1…1) plus idle **micro-saccades**.

Expression-specific choices are coded in `fillTargetFor()` in `eve_expression_state.cpp` (spacing, tilt, squint, scan strength, etc.).

## 5. How system state drives the state machine

**Automatic priority (see `pickAuto()`):**

1. Recent **record failure** → `CONFUSED` (timed).
2. **Critical battery** → `LOW_BATTERY`.
3. **Searching for WALL‑E** flag → `SEARCHING_FOR_WALLE` (animated gaze).
4. **Tracking** flag → `TRACK_TARGET` (uses `eveExpressionSetTargetGaze`).
5. **Docked + charging** → after ~8 s stillness, `SLEEP`; else `SOFT_IDLE`.
6. **Reconnect window** (after `eyesNotifyWallEConnected`) → short `AFFECTION`.
7. **WALL‑E disconnected** → `CONCERNED`.
8. **Attached** (present pin) → `SOFT_IDLE`.
9. **Legacy `eyesSetMode`** → `ALERT` / `HAPPY` bias.
10. Default → `NEUTRAL_IDLE`.

**Overrides:** `eveExpressionRequest(id, holdMs)` forces an expression for a timed window (bench keys and internal cues).

**Wiring today:**

- `state_machine.cpp`: `eyesNotifyWallEConnected` after `MSG_WALL_E_ACK`; `eyesNotifyWallEDisconnected` on link timeout; `eyesNotifyDockingState` on `MSG_MODE_DOCK` / `MSG_MODE_IDLE` (charging inferred from current).
- `eve_behavior_manager.cpp`: `eyesNotifySharedVoicebox` from remote sound tracks 3–4.
- `eyesSetMode` / `eveExpressionSetLegacyMode` from attachment + sound behavior.
- **Call `eyesNotifyRecordFailure()`** from your UART/menu path when RECORD is unavailable.

**Future hooks:** vision can call `eveExpressionSetTracking(true)` and `eveExpressionSetTargetGaze(nx, ny)`; ToF proximity can nudge the same.

## 6. Debug & validation checklist

**Serial tags**

- `[EVE_FACE] LVGL face initialized`
- `[EVE_FACE] Expression -> …` (on change)
- `[EVE_FACE] Blink triggered` / `Double blink`
- `[EVE_FACE] WALL-E connected, affection response` / `disconnected`
- `[EVE_FACE] Record unavailable -> confused`
- `[EVE_FACE] Shared voicebox on/off`
- `[EVE_FACE] Expression request -> …` (bench / one-shots)

**Bench mode (`EVE_FACE_DEBUG_BENCH` = 1):** send serial keys (`?` = help): `0–9` expressions, `n` reconnect, `w` disconnect, `p` record fail, `s`/`S` search, `h` dock+charge, `c` undock, `l/r/u/d` gaze.

| Check | Pass criteria |
|--------|----------------|
| Boot face | With `EVE_ENABLE_EYES` 1, visor + two eyes appear; log shows LVGL initialized. |
| Idle animation | Micro saccades + scan bar drift; periodic blinks. |
| Blink | Log `Blink triggered`; lids close briefly without full-screen flicker. |
| Gaze shifts | Bench `l/r/u/d` or `TRACK` + `SetTargetGaze`. |
| Affection / reconnect | Bench `n` or real `MSG_WALL_E_ACK` → softer, brighter eyes briefly. |
| Confused | Bench `p` or `6` → tilt / asym scan feel. |
| Low battery | Bench `9` or real critical % → narrow, dim, heavy lids. |
| Sleep / wake | Bench `2` then `3` (wake open animation) or dock idle 8s + charging. |
| Dock / charge | Bench `h` / `c` or UART dock modes; charging uses current threshold 30 mA (tunable). |

---

**Success target:** One modular, LVGL‑9-native EVE face: readable at arm’s length, minimal silhouette, motion-led emotion, non-blocking timing, and a clear path to wire vision, UART commands, and dual-display mirroring later.
