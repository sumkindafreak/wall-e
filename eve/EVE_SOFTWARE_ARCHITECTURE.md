# EVE software architecture

**Architecture status:** **Locked** after PR #7. The layer stack does not change; the robot evolves **inside** it. Phase N (Eye Controller) is **mature** — Phase R polish only.

Nothing in this document replaces existing phase notes (J–N). It defines how future work **slots into layers** without blurring responsibilities.

---

## Three milestones

| Milestone | Question | Status |
|-----------|----------|--------|
| **V1 — Body** | *How is EVE constructed?* | **Complete** — ESP32-S3, dual eyes, SD assets, native I2S, Eye Controller, emotion/gaze stack, CI, build matrix. The robot **functions**. |
| **V2 — Brain** | *How does EVE think?* | **Specified** — Awareness → Behaviour → Emotion → Outputs. Phases O–R implement the brain. Every piece of logic has a **home**. |
| **V3 — Character** | *Who is EVE?* | **Phase S** — Character System (personality, mood, preferences). Same firmware, different SD profile → different **companion**. **Presence**, not just behaviour. |

Up to V1: building a **robot**. After Phase O: building **behaviour**. After Phase S: building **presence**.

---

## EVE V2 — five layers (locked)

Sensing → thinking → feeling → expressing.

```text
                    HARDWARE
        (ToF, Battery, Wi-Fi, Dock, Audio, LCDs)
                         │
                         ▼
                  AWARENESS LAYER
          (Facts only — no decisions, no emotion)
                         │
                         ▼
             BEHAVIOUR INTELLIGENCE
          (Intent, priorities, active behaviour)
                         │
                         ▼
                 EMOTION ENGINE
       (Maps behaviour → affect and output hints)
                         │
          ┌──────────────┴──────────────┐
          ▼                             ▼
   EYE CONTROLLER                 AUDIO ENGINE
          │                             │
          ▼                             ▼
     Dual Eye LCDs                 Speaker / I2S
```

**Emotion tells both output systems what to express.** Neither eyes nor audio know *why* they are happy.

Example:

```text
Behaviour:  Greeting
Emotion:    Happy
Outputs:
  Eyes   → happy expression, slight blink, look at person
  Audio  → hello_02.wav
```

Later outputs (head tilt, body, arms, lighting) attach at the same level as eyes and audio.

**Future inputs plug into Awareness without redesign:**

- Vision module → Awareness
- Speech recognition → Awareness
- Local language model → Behaviour (intent), not Emotion or Outputs directly

---

## Dependency direction (hard rule)

> **A layer may depend only on the layer immediately below it, never above it.**

Data and control flow **down** the stack. No layer reaches up.

### Allowed

```text
Hardware
    ↓
Awareness
    ↓
Behaviour
    ↓
Emotion
    ↓
Outputs (Eye Controller, Audio Engine, …)
```

### Forbidden

```text
Emotion → Awareness
Eye Controller → Behaviour
Audio → Behaviour
Display → ToF
Behaviour → SPI
Behaviour → LVGL
Awareness → Expression
```

Examples of violations to reject in review:

- Emotion reading ToF directly instead of consuming Behaviour intent
- Eye Controller choosing Curious vs Idle
- Audio engine triggering a behaviour transition
- Behaviour calling SPI flush or `lv_obj_*`

This rule prevents architectural drift as the project grows.

---

## One public API per layer

Each layer exposes **one primary surface** to the layer above. Callers do not reach inside subsystems — everything stays replaceable.

| Layer | Public API (target) | Consumes |
|-------|---------------------|----------|
| **Awareness** | `const EveAwarenessSnapshot& eveAwarenessGetSnapshot()` | Hardware drivers only |
| **Character** | `const EveCharacterState& eveCharacterGetState()` | Awareness snapshot (Phase S / V3) |
| **Behaviour** | `const EveBehaviourState& eveBehaviourGetState()` | Awareness + Character biases |
| **Emotion** | `const EveEmotionState& eveEmotionGetState()` (affect + output hints) | Behaviour state |
| **Eye Controller** | `eveEyesTick()` | Emotion output hints |
| **Audio Engine** | `eveAudioTick()` | Emotion output hints |

Character is not a stack layer above Behaviour in the dependency diagram — it is a **read-only modifier** Behaviour consults. It must not call Emotion or Outputs.

---

## Layer responsibilities

| Layer | Owns | Must not own |
|-------|------|--------------|
| **Hardware** | Sensors, SPI displays, SD, I2S, power, UART | Software policy |
| **Awareness** | Unified snapshot of world + system facts | Intent, expression, playback |
| **Behaviour** | Active intent, transitions, one behaviour at a time | Rendering, WAV paths, lid curves |
| **Emotion** | Affect from behaviour (expression, gaze hints, audio hints) | ToF decisions, personality FSM |
| **Outputs** | Eyes (Eye Controller), Audio (I2S queue), future actuators | Interpreting sensors |

---

## Awareness

**Name:** Awareness (not “perception”) — EVE is not doing SLAM, mapping, or vision yet. This layer answers *what is true right now*, not *what should we do*.

Publishes one **immutable snapshot** per update (see Phase O). Example fields:

- `personPresent`, `personDistance`, `personZone`, `motionDetected`
- `batteryLevel`, `batteryLow`, `charging`, `docked`
- `wallELinked`, `voiceDetected`, `ambientNoise`
- Timestamps and confidence where applicable

No emotion. No behaviour selection. No drawing.

**Current code (migration):** `eve_target_tracker`, `eve_spatial_awareness`, battery/dock flags, UART link state — these feed Awareness; they must not drive expression directly in V2.

---

## Behaviour

**Name:** behaviours (not “behaviour modules”). Each behaviour implements:

```cpp
enter(AwarenessSnapshot)
update(AwarenessSnapshot, dt)
exit()
```

Only **one** behaviour is active. The Behaviour Manager owns registration, switching, and tick order.

**Character System (Phase S / V3)** sits **above Behaviour logic, below Emotion**: Personality (long-term) + Mood (short-term) **bias** thresholds and timings — they do not replace behaviours or bypass Awareness. See `PHASE_S_CHARACTER_SYSTEM.md`.

Planned behaviours:

| Behaviour | Role |
|-----------|------|
| **Idle** | Default; low activity, ambient presence |
| **Curious** | Investigate motion / new stimulus |
| **Follow** | Sustained interest in a person or zone |
| **Greeting** | Close approach, social opening |
| **Conversation** | Voice / WALL-E audio / listen mode |
| **Sleep** | Low power, docked, or prolonged inactivity |

Behaviour chooses **intent**. It does not set LVGL objects or call `audioPlayTrack()` with hard-coded policy (it requests outcomes through Emotion).

**Current code (migration):** `eve_behavior_manager` is largely a stub; `eve_emotion_engine` still contains ToF/person transition logic — that moves to Behaviour in Phase P; Emotion slimming follows in Phase Q.

---

## Emotion

Maps **behaviour → affect**. Examples:

```text
Behaviour = Follow    →  Emotion = Curious  (+ track gaze hint)
Behaviour = Sleep     →  Emotion = Sleepy   → Sleep
Behaviour = Greeting  →  Emotion = Happy
```

Produces an **affect request** consumed by outputs:

- Eye hints: expression id, look direction, blink flags (via Eye Controller / `eveEyeControllerApplyRequest` or successor)
- Audio hints: cue id or asset path, volume bias, queue policy

Emotion is a **result**, not a controller. Avoid:

```text
Person detected → Emotion decides → everything else   ❌
```

Prefer:

```text
Person detected → Awareness → Behaviour decides → Emotion → outputs   ✓
```

**Current code (migration):** Phase L emotion FSM and orchestrator remain useful as **affect** machinery; world-driven transitions move up to Behaviour.

---

## Outputs

### Eye Controller (mature — Phase N)

Sole owner of left/right rendering. Layer stack, dual CS, blink engine. Phase R adds polish only (micro-saccades, async blinks, dwell) **inside** this layer.

### Audio Engine (Phase M baseline)

SD → WAV → I2S. Queue, volume, callbacks. Plays what Emotion requests; does not interpret ToF.

### Future

Head tilt, body motion, arm gestures, lighting — same sibling pattern under Emotion.

---

## Tick order (target V2)

```text
1. Hardware read / drivers
2. Awareness tick        → publish EveAwarenessSnapshot
3. Character tick        → update mood from snapshot; personality from SD (Phase S)
4. Behaviour Manager     → active behaviour update (character-biased thresholds) → intent
5. Emotion               → affect request from intent
6. Eye Controller tick   → render from affect + hints
7. Audio Engine tick     → queue from affect + hints
```

Steps 3–4: Character **biases** Behaviour; it does not choose expressions or play audio. Eye Controller may still run internal sub-ticks (gaze ease, blink) as **output refinement**, not as world decisions.

---

## Phase roadmap

| Phase | Milestone | Focus |
|-------|-----------|--------|
| **O** | V2 | Awareness — sensor fusion, unified snapshot, confidence, no decisions |
| **P** | V2 | Behaviour intelligence — Idle, Curious, Follow, Greeting, Sleep (+ Conversation) |
| **Q** | V2 | Emotion refactor — remove world logic; affect + eye/audio hints only |
| **R** | V2 | Expression polish — micro-saccades, async blinks, gaze easing, dwell |
| **S** | **V3** | **Character System** — personality (bias, not fork), mood, preferences |

Display architecture is **frozen** after Phase N except for Phase R polish inside Eye Controller.

### Phase S — Character System (V3)

Not a new layer in the stack — a **modifier** consumed by Behaviour only.

| Component | Horizon | Role |
|-----------|---------|------|
| **Personality** | Long-term (SD profile) | Biases interest scores, delays, intervals — same behaviour classes |
| **Mood** | Short-term (from Awareness) | Tired, alert, social — tempers how personality expresses |
| **Preferences** | Future | Likes/dislikes; still feeds Behaviour biasing |

Example: high **curiosity** raises effective interest before Investigate threshold; high **shyness** multiplies greet delay. **Mood: tired** (low battery) slows investigation without erasing a curious personality.

See `PHASE_S_CHARACTER_SYSTEM.md`.

---

## V1 — Body (complete)

Platform (pioarduino 3.3.9, CI, artifacts, versioning), SD asset manager, native I2S audio (DFPlayer removed), emotion/gaze/idle engines (Phase L), dual Eye Controller (Phase N). See `PHASE_J` through `PHASE_N` docs.

---

## Design discipline

New features should land in **exactly one** layer:

- New sensor → Awareness
- “She noticed me” → Behaviour
- “She looks happy” → Emotion
- “Eyelids move” / “Sound plays” → Outputs
- “She’s shy vs bold” → Character / Personality (Phase S, biases Behaviour)
- “She’s tired right now” → Character / Mood (Phase S, from Awareness)

If a change touches two layers, split the PR or document the boundary explicitly.

**Foundation is stable.** From here: behaviour, expression, character — not structural redesign.
