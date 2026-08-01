# Phase O-2 — ToF → Awareness (person facts)

**Goal:** EVE **notices** a person via ToF for the first time. Publish exactly four person fields. Nothing reacts.

O-1 snapshot struct remains **frozen** except documented zone constants. No new person fields.

---

## Inputs

**ToF subsystem only** (`eve_tof_manager` raw frame).

No battery, emotion, eyes, target tracker L/C/R model, or behaviour flags in `publishPersonFacts()`.

---

## Outputs (person block only)

| Field | Rule |
|-------|------|
| `personDistanceMm` | Closest valid ToF reading (mm), or `-1` |
| `personZone` | `EveAwarenessPersonZone` from distance bands |
| `personConfidence` | `100` if valid reading, else `0` |
| `personPresent` | **Derived:** `personConfidence >= EVE_AWARENESS_PERSON_PRESENT_THRESHOLD` |

Behaviour must compare **zones**, never raw millimetres:

```cpp
if (snapshot.personZone == EVE_AWARENESS_ZONE_NEAR) { ... }
```

---

## Zones (frozen enum)

```text
EVE_AWARENESS_ZONE_UNKNOWN = 0
EVE_AWARENESS_ZONE_FAR
EVE_AWARENESS_ZONE_MID
EVE_AWARENESS_ZONE_NEAR
EVE_AWARENESS_ZONE_PERSONAL
```

Distance bands are **config-only** (`config.h`):

| Band | Default upper bound (mm) |
|------|--------------------------|
| PERSONAL | ≤ 550 |
| NEAR | ≤ 900 |
| MID | ≤ 1500 |
| FAR | < `EVE_TOF_FAR_IGNORE_MM` (2400) |
| UNKNOWN | invalid / out of range |

---

## Tick order

`main.cpp` runs **`tofTick()` before `eveAwarenessTick()`** so Awareness reads the frame polled in the same loop iteration.

---

## Acceptance test (bench)

Walk toward EVE, then leave. Serial `AWARENESS` block only — **no** blink, greeting, sound, or expression changes.

| Step | Present | Distance | Zone | Confidence |
|------|---------|----------|------|------------|
| ~2 m | NO | — | UNKNOWN | 0 |
| ~1 m | YES | ~2012 mm | FAR | 100 |
| ~500 mm | YES | ~1018 mm | MID | 100 |
| ~250 mm | YES | ~482 mm | PERSONAL | 100 |
| leave | NO | — | UNKNOWN | 0 |

Enable hardware ToF (`EVE_ENABLE_TOF=1`) or bench simulate (`EVE_TOF_SIMULATE=1` with scripted distances).

Host-side zone mapping check:

```bash
cd eve && g++ -std=c++11 -Wall -Wextra -o /tmp/awareness_o2_zones_test test/awareness_o2_zones_test.cpp && /tmp/awareness_o2_zones_test
```

---

## Out of scope (O-2)

- Behaviour, emotion, eyes, audio policy changes
- Target tracker / spatial ack / head tracking reads from Awareness
- Removing `eveEmotionOnTofSnapshot()` (Phase P/Q migration)
- Extra snapshot fields “just because”

---

## Success criteria

- [x] `publishPersonFacts()` reads `eveTofManagerGetLastFrame()` only
- [x] Four person outputs; `personPresent` derived from confidence
- [x] Zone enum + config thresholds; serial prints zone names
- [x] `tofTick()` before `eveAwarenessTick()`
- [x] All firmware targets compile with `EVE_ENABLE_TOF=0` (neutral person stub)
