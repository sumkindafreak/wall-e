# Phase O — Awareness layer

**Goal:** One place for **facts** about the world and EVE’s own state. No intent, no emotion, no rendering, no audio policy.

---

## Why “Awareness”

“Perception” often implies SLAM, vision pipelines, or mapping. EVE’s near-term inputs are ToF, battery, dock/link UART, and (future) microphone level. **Awareness** describes sensor fusion into a snapshot without overstating capability.

---

## Architecture

```text
Hardware (ToF, INA219, UART, mic*, GPIO)
        │
        ▼
   Awareness tick
        │
        ▼
EveAwarenessSnapshot  (immutable per frame)
        │
        ▼
   Behaviour Manager only (primary consumer)
```

\* Microphone / ambient noise when hardware exists.

---

## Snapshot (target API)

Single struct published each update. Behaviour always reads a **complete** snapshot — easier to test, log, and replay.

```cpp
typedef struct {
  uint32_t timestampMs;

  /* Person / motion (from ToF + tracker) */
  bool personPresent;
  float personDistanceMm;      /* or -1 if unknown */
  uint8_t personZone;          /* maps from EveTargetModel */
  uint8_t personConfidencePct;
  bool motionDetected;

  /* Power / dock */
  float batteryPercent;
  bool batteryLow;
  bool charging;
  bool docked;

  /* Link / social */
  bool wallELinked;
  bool voiceDetected;          /* local or remote cue */
  float ambientNoiseDb;        /* optional, 0 if unavailable */

  /* System (optional flags) */
  bool alertRequested;         /* e.g. low battery latch from hardware */
} EveAwarenessSnapshot;
```

Rules:

- **No opinions** — do not set “curious” or “should greet” here.
- **Confidence** on derived fields where fusion is noisy (ToF zones, person presence).
- **Copy-out** — consumers get a const snapshot; Awareness does not expose mutable globals.

Suggested accessors:

```cpp
void eveAwarenessInit(void);
void eveAwarenessTick(uint32_t nowMs);
void eveAwarenessGetSnapshot(EveAwarenessSnapshot* out);
```

---

## Sensor fusion (incremental)

| Source | Feeds |
|--------|--------|
| `eve_tof_manager` / raw frames | distance, motion |
| `eve_target_tracker` | zone, confidence, stableStrong |
| Battery / INA219 | percent, low, charging |
| Dock / UART / shared behaviour | docked, wallELinked |
| Future mic | voiceDetected, ambientNoise |

Phase O **wraps** existing modules; it does not replace ToF drivers. It replaces ad-hoc reads and emotion-side ToF rules with one publish point.

---

## Migration from V1

| Today | Phase O |
|-------|---------|
| `eveTargetTrackerGetSnapshot()` | Input to Awareness fusion |
| `eve_spatial_awareness` flags | Fold into snapshot or Awareness internals |
| `eveEmotionOnTofSnapshot()` | **Remove** from Emotion; Behaviour reads snapshot |
| Scattered `config` / dock flags | Normalized in snapshot |

---

## Testing

- **Replay:** Feed recorded snapshots into Behaviour unit tests (Phase P) without hardware.
- **Bench:** Serial command to dump snapshot JSON (optional, behind `EVE_ENABLE_SERIAL_CONSOLE`).
- **CI:** Build-only with `EVE_ENABLE_TOF=0`; stub snapshot with safe defaults.

---

## Success criteria

- [ ] `EveAwarenessSnapshot` defined in `eve/include/`
- [ ] `eveAwarenessTick()` fuses ToF + battery + link when enabled
- [ ] No new decision logic in Awareness
- [ ] Emotion engine **unchanged** in Phase O (Behaviour migration is Phase P)
- [ ] All firmware targets still compile

---

## Out of scope (Phase O)

- Behaviour FSM (Phase P)
- Emotion refactor (Phase Q)
- Eye/audio changes (Phase R)
- SLAM, camera, ML
