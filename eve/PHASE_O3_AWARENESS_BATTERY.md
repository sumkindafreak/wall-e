# Phase O-3 — Battery → Awareness (power facts)

**Status: frozen.** Passed — exactly five battery facts, nothing else changed.

**Goal:** EVE knows **"this is my current energy state."** Not when to sleep. Not warnings. Not faces.

---

## Inputs

**Battery subsystem only** (`battery_monitor` / INA219).

No ToF, dock UART, emotion, eyes, or behaviour in `publishBatteryFacts()`.

---

## Outputs (battery block only)

| Field | Rule |
|-------|------|
| `batteryVoltage` | Pack voltage (V), or `0` if invalid |
| `batteryPercent` | SoC `0–100`, or `-1` if unknown |
| `batteryLow` | **Derived:** `batteryPercent <= EVE_AWARENESS_BATTERY_LOW_PCT` |
| `charging` | **Derived:** valid data and `currentA > EVE_AWARENESS_CHARGING_MIN_A` |
| `batteryHealth` | **Derived:** `EveBatStatus` → `EveAwarenessBatteryHealth` |

### Not in Awareness

```cpp
goSleep()          // Behaviour (Phase P)
playWarning()      // Emotion / Audio
showBatteryFace()  // Eye Controller
```

---

## Acceptance test (bench)

Watch serial `AWARENESS` only. Robot does **absolutely nothing**.

```text
Plug charger in.
↓
charging = true
↓
Unplug charger.
↓
charging = false
↓
Lower battery.
↓
batteryPercent decreases
↓
batteryLow eventually true
↓
Robot does absolutely nothing.
```

Success.

Host helpers:

```bash
cd eve && g++ -std=c++11 -Wall -Wextra -I include -I awareness \
  -o /tmp/awareness_o3_battery_test test/awareness_o3_battery_test.cpp && \
  /tmp/awareness_o3_battery_test
```

---

## Tick order

`eveBatteryTick()` → … → `tofTick()` → `eveAwarenessTick()` (unchanged from O-2).

---

## Out of scope

- Sleep behaviour, LED/audio/expression changes
- Person fields (O-2, frozen)
- Behaviour (Phase **P-1 Observe** comes after O-4/O-5 per roadmap pact)

---

## Success criteria

- [x] `publishBatteryFacts()` reads `battery_monitor` only
- [x] Five battery outputs; `batteryLow` and `charging` derived
- [x] Append-only: `batteryPercent` + `batteryHealth` added; existing names unchanged
- [x] No behaviour/emotion/eye/audio changes
- [x] All firmware targets compile
