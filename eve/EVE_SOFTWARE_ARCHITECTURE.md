# EVE software architecture

**Read first:** [`EVE_VISION.md`](EVE_VISION.md) — why this stack exists (companion coherence, not animatronics).

**Architecture status:** **Locked**. The stack does not get redesigned — EVE evolves **inside** it through **experiences** (behaviour, presence, continuity). Phase N (Eye Controller) is **mature** — Phase R polish only.

The question shifts from *"what should the architecture be?"* to *"what experiences should EVE create?"*

Nothing in this document replaces existing phase notes (J–N). It defines how future work **slots into layers** without blurring responsibilities.

---

## Four milestones

Each rung supports the next; no phase invalidates the previous.

| Milestone | Tagline | Question | Status |
|-----------|---------|----------|--------|
| **V1 — Body** | Can move and express | *How is EVE constructed?* | **Complete** — hardware, eyes, audio, CI, build matrix |
| **V2 — Mind** | Can understand and decide | *How does EVE think?* | **Phases O–R** — Awareness → Behaviour → Emotion → Outputs |
| **V3 — Character** | Can feel unique | *Who is EVE?* | **Phase S** — personality, mood, preferences → **presence** |
| **V4 — Memory** | Can remember | *What happened before?* | **Phase T** — continuity via context into Character |

| Phase of project | Focus |
|------------------|--------|
| Through V1 | Building a **robot** |
| Phases O–P | Building **behaviour** |
| Phase S | Building **presence** |
| Phase T | Building **continuity** |

---

## Three timescales

Each layer runs on a different clock (see `EVE_VISION.md`):

| Timescale | Examples | Primary layers |
|-----------|----------|----------------|
| **Milliseconds** | Blink, gaze ease, WAV playback | Outputs |
| **Seconds** | Greet, follow, investigate, conversation | Awareness, Behaviour, Emotion |
| **Hours / days** | Remembered interactions, personality profile | Memory, Character |

Do not mix clocks (e.g. long-term recall inside blink timing).

---

## Full pipeline (Mind + Character + Memory)

Core **Mind** stack (locked). **Memory** and **Character** are context modifiers before Behaviour — not decision layers.

```text
                    HARDWARE
        (ToF, Battery, Wi-Fi, Dock, Audio, LCDs)
                         │
                         ▼
                  AWARENESS LAYER
              (Facts now — no decisions)
                         │
                         ▼
                    MEMORY
         (Context — what was true before)
              does not decide
                         │
                         ▼
                  CHARACTER SYSTEM
            Personality + Mood (+ memory bias)
                         │
                         ▼
             BEHAVIOUR INTELLIGENCE
                         │
                         ▼
                 EMOTION ENGINE
                         │
          ┌──────────────┴──────────────┐
          ▼                             ▼
   EYE CONTROLLER                 AUDIO ENGINE
```

**Character** answers *"Who am I?"* **Memory** answers *"What do I remember?"* Behaviour still owns intent; Emotion still owns affect; Outputs still render and play.

---

## EVE V2 — Mind layers (locked core)

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
- **LLM / language model → Awareness (facts) or optional advisor — not Behaviour replacement**

The Mind stack stays **deterministic, predictable, debuggable**. That is a strength.

---

## Memory (design philosophy — Phase T / V4)

**Not another decision layer.** Memory provides **continuity** — context for Character so repeated interactions feel considered.

Example: person returns 30 s after a greeting → Memory recalls recent greet → Character biases toward smile-without-replay → Behaviour skips full Greeting → Emotion still maps warm affect → eyes brighten, no duplicate hello WAV.

Stored on SD (`/memory` or small JSON). Lightweight `RememberedEvent` records with **expiry**. See `PHASE_T_MEMORY.md`.

**Forbidden:** Memory → Behaviour, Memory → Emotion, Memory → Outputs directly.

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
Memory          (Phase T — context only)
    ↓
Character       (Phase S — personality + mood + memory bias)
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
Memory → Behaviour
Memory → Emotion
Memory → Outputs
Character → Emotion
Character → Outputs
Mood → Eye Controller
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
| **Memory** | `const EveMemoryContext& eveMemoryGetContext()` | Awareness snapshot (Phase T / V4) |
| **Character** | `const EveCharacterState& eveCharacterGetState()` | Awareness + Memory context |
| **Behaviour** | `const EveBehaviourState& eveBehaviourGetState()` | Awareness + Character biases |
| **Emotion** | `const EveEmotionState& eveEmotionGetState()` (affect + output hints) | Behaviour state |
| **Eye Controller** | `eveEyesTick()` | Emotion output hints |
| **Audio Engine** | `eveAudioTick()` | Emotion output hints |

Character and Memory are **read-only modifiers** on the path to Behaviour. They must not call Emotion or Outputs.

---

## Layer responsibilities

| Layer | Owns | Must not own |
|-------|------|--------------|
| **Hardware** | Sensors, SPI displays, SD, I2S, power, UART | Software policy |
| **Awareness** | Unified snapshot of world + system facts | Intent, expression, playback, long-term recall |
| **Memory** | SD-backed events, recency, session stats | Decisions, expression, direct Behaviour calls |
| **Character** | Personality, mood, memory-informed bias | Behaviour selection, rendering |
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
3. Memory tick           → recall, expire events, debounced SD persist (Phase T)
4. Character tick        → personality + mood + memory context (Phase S)
5. Behaviour Manager     → character-biased thresholds → intent
6. Emotion               → affect request from intent
7. Eye Controller tick   → render from affect + hints
8. Audio Engine tick     → queue from affect + hints
```

Steps 3–4: Character **biases** Behaviour; it does not choose expressions or play audio. Eye Controller may still run internal sub-ticks (gaze ease, blink) as **output refinement**, not as world decisions.

---

## Phase roadmap

| Phase | Milestone | Focus |
|-------|-----------|--------|
| **O** | V2 | Awareness — O-1…O-5 incremental publishers; append-only snapshot; **complete at O-5** |
| **P** | V2 | Behaviour — **P-1 Observe** (`IdleBehaviour`), then Curious, Follow, Greeting, … |
| **Q** | V2 | Emotion refactor — remove world logic; affect + eye/audio hints only |
| **R** | V2 | Expression polish — micro-saccades, async blinks, gaze easing, dwell |
| **S** | **V3** | **Character System** — personality (bias, not fork), mood, preferences |
| **T** | **V4** | **Memory** — continuity, remembered events, context into Character |

Display architecture is **frozen** after Phase N except for Phase R polish inside Eye Controller.

### Phase S — Character System (V3)

See `PHASE_S_CHARACTER_SYSTEM.md`.

### Phase T — Memory (V4)

Context only — feeds Character, not Behaviour. See `PHASE_T_MEMORY.md`.

---

## V1 — Body (complete)

Platform (pioarduino 3.3.9, CI, artifacts, versioning), SD asset manager, native I2S audio (DFPlayer removed), emotion/gaze/idle engines (Phase L), dual Eye Controller (Phase N). See `PHASE_J` through `PHASE_N` docs.

---

## Design discipline

### Which layer owns the decision?

> **When adding a new feature, first decide which layer owns the decision — not which file should contain the code.**

| Example | Layer |
|---------|--------|
| Battery low | Awareness |
| Looks tired | Emotion |
| Chooses to dock | Behaviour |
| Remembers yesterday | Memory |
| Waits longer before greet (shy) | Character |

### Where features land

New features should land in **exactly one** layer:

- New sensor → Awareness
- “She noticed me” → Behaviour
- “She looks happy” → Emotion
- “Eyelids move” / “Sound plays” → Outputs
- “She’s shy vs bold” → Character / Personality (Phase S)
- “She’s tired right now” → Character / Mood (Phase S)
- “She already said hello” → Memory → Character (Phase T)
- “She remembers yesterday” → Memory (Phase T)

If a change touches two layers, split the PR or document the boundary explicitly.

**Foundation is stable.** Design **experiences** — behaviour, presence, continuity — not new layers.
