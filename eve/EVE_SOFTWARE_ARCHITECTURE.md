# EVE software architecture (V1 baseline → V2 brain)

Version 1 established the **output stack**: independent eyes, native I2S audio, SD assets, and a clean render path. Version 2 adds the **brain stack**: awareness of the world, behavioural intent, and affect that drives expressive outputs.

Nothing in this document replaces existing phase notes (J–N). It defines how future work **slots into layers** without blurring responsibilities.

---

## Five layers

```text
           Hardware
               │
               ▼
        Awareness Layer          ← facts only (no opinions)
               │
               ▼
      Behaviour Intelligence     ← intent (one active behaviour)
               │
               ▼
        Emotion Engine           ← affect mapping (not world logic)
               │
        ┌──────┴──────┐
        ▼             ▼
 Eye Controller   Audio Engine    ← outputs (parallel siblings)
        │             │
        ▼             ▼
   Left / Right     I2S + WAV
   eye panels
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
3. Behaviour Manager     → active behaviour update → intent
4. Emotion               → affect request from intent
5. Eye Controller tick   → render from affect + hints
6. Audio Engine tick     → queue from affect + hints
```

Eye Controller may still run internal sub-ticks (gaze ease, blink) as **output refinement**, not as world decisions.

---

## Phase roadmap (post V1)

| Phase | Focus |
|-------|--------|
| **O** | Awareness layer — sensor fusion, unified snapshot, confidence, no decisions |
| **P** | Behaviour intelligence — Idle, Curious, Follow, Greeting, Sleep (+ Conversation) |
| **Q** | Emotion refactor — remove world logic; affect + eye/audio hints only |
| **R** | Expression polish — micro-saccades, async blinks, gaze easing, dwell, idle refinement |

Display architecture is **frozen** after Phase N except for Phase R polish inside Eye Controller.

---

## V1 completed (reference)

Platform (pioarduino 3.3.9, CI, artifacts, versioning), SD asset manager, native I2S audio (DFPlayer removed), emotion/gaze/idle engines (Phase L), dual Eye Controller (Phase N). See `PHASE_J` through `PHASE_N` docs.

---

## Design discipline

New features should land in **exactly one** layer:

- New sensor → Awareness
- “She noticed me” → Behaviour
- “She looks happy” → Emotion
- “Eyelids move” / “Sound plays” → Outputs

If a change touches two layers, split the PR or document the boundary explicitly.
