# Phase O-3 — Battery → Awareness (power facts)

**Goal:** EVE knows her **own power state** as structured facts. Same philosophy as O-2: one subsystem in, facts out. Nothing reacts.

O-1/O-2 remain **frozen**. O-3 **appends** battery fields; it does not rename or overload existing facts.

---

## Inputs

**Battery subsystem only** (`battery_monitor` / INA219).

No ToF, emotion, eyes, behaviour, or dock UART in `publishBatteryFacts()`.

(`docked` stays a separate publisher — dock/link facts, not battery chemistry.)

---

## Outputs (battery block)

| Field | Rule |
|-------|------|
| `batteryVoltage` | Pack voltage (V), or `0` if invalid |
| `batteryPercent` | SoC estimate `0–100`, or `-1` if unknown |
| `batteryLow` | **Derived:** from percent/status thresholds (config) |
| `charging` | **Derived:** docked + positive charge current (fact, not policy) |
| `batteryHealth` | **Derived:** coarse health band from status / trend (see below) |

### Not in Awareness

Awareness must **never** call or imply:

```cpp
goSleep()
playWarning()
showBatteryFace()
```

Those are Behaviour / Emotion / Output decisions in Phase P+.

---

## Derived fields (same pattern as O-2)

```cpp
batteryLow = (batteryPercent >= 0 && batteryPercent <= EVE_BAT_CRIT_PCT)
          || eveBatteryStatus() >= EVE_BAT_WARN;
// charging: factual INA219 current + dock state (already in O-1 stub — refine in O-3)
batteryHealth: map EveBatStatus → stable enum (OK / WARN / CRITICAL / UNKNOWN)
```

Behaviour reads `snapshot.batteryLow`; it does not re-derive thresholds from raw voltage.

---

## Append-only contract

When O-3 lands, **add** `batteryPercent` and `batteryHealth` to the snapshot.

Do **not** rename `batteryVoltage` or overload `batteryLow` with new meaning.

If a future field is needed (e.g. `batteryCurrentA`), **add** it — do not repurpose an existing name.

---

## Acceptance test (bench)

Serial `AWARENESS` block only. No sleep mode, warning chirp, or battery face.

| Condition | Voltage | Percent | Low | Charging | Health |
|-----------|---------|---------|-----|----------|--------|
| Full, on bench | ~8.2 V | ~100 | NO | NO | OK |
| Mid pack | ~7.4 V | ~50 | NO | NO | OK |
| Below warn | ~6.8 V | ~20 | YES | NO | WARN |
| Critical | ~6.2 V | ~10 | YES | NO | CRITICAL |
| On dock, charging | stable | rising | NO | YES | OK |

Tune thresholds in `config.h`; Behaviour never reads raw ADC.

---

## Out of scope (O-3)

- Sleep behaviour, audio warnings, expression changes
- Dock/link fields (separate publisher)
- Person / ToF fields (O-2, frozen)

---

## Success criteria

- [ ] `publishBatteryFacts()` reads `battery_monitor` only
- [ ] Five battery outputs; derived fields documented
- [ ] `eveBatteryTick()` before `eveAwarenessTick()` (already true in main loop)
- [ ] No behaviour/emotion/eye/audio changes
- [ ] All firmware targets compile with `EVE_ENABLE_BATTERY_MONITOR=0` (safe stubs)
