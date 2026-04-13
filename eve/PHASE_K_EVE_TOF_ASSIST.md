# Phase K — EVE ToF awareness, acknowledgement, WALL-E drive assist

## 1. Scan: existing ToF / servos / ownership

| Item | Finding |
|------|--------|
| **Prior code** | `tof_control.cpp` was an empty stub; `config.h` documents **dual VL53L1X** on **shared I2C** (`EVE_I2C_SDA` / `SCL`), with optional **XSHUT** on the second unit for addressing. |
| **Mounting (intended)** | **Left** sensor → physical left FOV; **Right** sensor → physical right. Optional **center** flag `EVE_TOF_HAS_CENTER` for a third channel (not wired in default config). |
| **Update rate** | `EVE_TOF_POLL_MS` (default **40 ms**). Continuous mode ~33 ms budget when VL53L1X is used. |
| **Raw layer** | `eve_tof_manager` — smoothing and validity in `eve_target_tracker`. |
| **Head output** | `EVE_SERVO_L_PIN` used as **head pan** via `servoSetHeadPanTarget()` (degrees **45–135**, neutral **90**). |
| **WALL-E link** | `uartLinkSendJson(MSG_EVE_TARGET_AWARENESS, …)` from `eve_target_relay`. |

**Hardware paths**

- **Simulate:** `EVE_ENABLE_TOF 1`, `EVE_TOF_SIMULATE 1`, I2C pins can stay `-1` (manager still runs cycle for bench).
- **Single VL53L1X:** Pololu library present, `EVE_TOF_XSHUT_SECOND < 0` → range fills **center** only (zones **CENTER / NONE**).
- **Dual VL53L1X:** `EVE_TOF_XSHUT_SECOND` defined → first sensor moved to `EVE_TOF_ADDR_FIRST`, second at `EVE_TOF_ADDR_SECOND`.

## 2. Zone model

| Derived state | Meaning |
|---------------|--------|
| `NONE` | No channel under `EVE_TOF_NEAR_MM` after smoothing + debounce. |
| `LEFT` | Left channel dominant / only left near. |
| `RIGHT` | Right channel dominant / only right near. |
| `CENTER` | Center (or single-sensor forward) near. |
| `MULTI` | Left and right both near and similar range (±120 mm). |
| `UNCERTAIN` | Conflicting center + side near. |

**Stability:** exponential smoothing on mm + **3-frame** agreement before published zone changes. **Confidence** 0–100 ramps up while zone holds, decays toward `NONE`.

## 3. ~1-in-5 acknowledgement

- **Probability** base **~18%** per eligible frame, **not** a fixed counter.
- **Cooldown** **2.2–5 s** random after a successful roll so EVE does not “machine-gun” turns.
- **Modifiers:** lower on `LOW_BATTERY` / `ATTACHED`; higher on `ALERT` (ESCORT/INTERACT) and when `stableStrong` (long hold + confidence).
- **Blocked:** `SLEEP`, `CONFUSED` (expression or flag), `NONE` / `UNCERTAIN`, low confidence.

## 4. Modules (EVE `eve/src`)

| File | Role |
|------|------|
| `eve_tof_manager` | Wire + optional Pololu VL53L1X (dual/single) or simulate. |
| `eve_target_tracker` | Smooth + classify + hysteresis + confidence. |
| `eve_acknowledgement_manager` | Probabilistic ack + cooldown. |
| `eve_head_tracking_manager` | `TRACK_*` states, soft pan, brief follow, return home; drives **LVGL gaze** when `EVE_ENABLE_EYES`. |
| `eve_target_relay` | JSON per `protocols/eve_target_protocol.h`, rate-limited UART. |
| `eve_spatial_awareness` | Pipeline + logs; calls `servoSetHeadPanTarget`. |

**WALL-E (`main_wall_E_base/main`)**

| File | Role |
|------|------|
| `eve_target_protocol.h` | Key / zone string constants. |
| `eve_target_assist` | Parse packets, freshness, suppress mask, motor delta. |
| `eve_uart_bridge` | `MSG_EVE_TARGET_AWARENESS` (0x08) → `eveTargetAssistIngestJson`. |
| `motor_control` | Adds assist delta inside `motorHandle()` ramp path. |

## 5. Safety / manual suppression

Assist **does not** replace higher-level control:

| Mask | When set (main loop) |
|------|----------------------|
| `EVE_ASSIST_MASK_SAFETY` | `unifiedAutonomySafetyActive()` |
| `EVE_ASSIST_MASK_DOCK` | Autonomous dock or dock homing driving motors |
| `EVE_ASSIST_MASK_MANUAL` | CYD or Web manual override |

When masked, bias decays and logs `[WALLE_ASSIST] Assist suppressed by safety` (safety) or similar behavior for dock/manual. Stale UART (> **1.6 s**) decays bias and logs stale ignore.

## 6. Validation checklist

| Test | Pass |
|------|------|
| Left detection | Dual sim / hardware: logs `[EVE_TOF] Left zone stable…`; relay `targetZone=LEFT`. |
| Right detection | Same for RIGHT. |
| Center | Single-sensor or symmetric MULTI → CENTER / MULTI. |
| ~20% ack | Many frames: visible head acks sparse; cooldown messages when skipping. |
| Follow | After ack, short follow then `TRACK_RETURN_HOME` to ~90° pan. |
| Cooldown | Repeated detections during cooldown: no new ack log. |
| Relay | WALL-E serial: `[WALLE_ASSIST] Received EVE target …`. |
| Drive bias | With autonomy driving, slight differential on motors when assist active. |
| Stale | Stop EVE UART: bias fades, stale log throttled. |
| Safety | Trip safety latch: assist suppressed log, delta ~0. |
| Manual | Move CYD sticks: assist suppressed, no creep from EVE. |

## JSON fields (EVE → WALL-E)

`proto`, `targetZone`, `confidencePct`, `distanceMm`, `ackTriggered`, `trackingActive`, `suggestedTurnBias`, `sourceNodeId`, `eventAgeMs`, `headPanDeg`, `nearThresholdMm`.

**Turn bias:** negative → soft **left** bias for WALL-E (left motor delta down, right up). Positive → soft **right**.
