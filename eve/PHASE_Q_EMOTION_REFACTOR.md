# Phase Q — Emotion refactor

**Goal:** Emotion becomes **affect only** — mapping Behaviour intent to expression, gaze hints, blink hints, and **audio hints**. Remove world logic (ToF, person timers, sleep policy) from the emotion engine.

Depends on **Phase P** (Behaviour intent).

---

## Architecture

```text
EveBehaviourIntent
        │
        ▼
   Emotion Engine
        │
        ├─► EveAffectOutput (eyes)
        │     expression, look, blink flags
        │
        └─► EveAffectOutput (audio)
              cue id / path, volume, queue mode
        │
        ▼
   Eye Controller          Audio Engine
   (parallel siblings)
```

Neither output subsystem knows **why** it is happy.

---

## Principles

1. **Emotion is a result, not a controller**
   - Wrong: person detected → emotion state CURIOUS → everything follows
   - Right: Behaviour Curious → Emotion applies Curious affect → outputs

2. **Critical overrides stay narrow**
   - Battery critical, record failure, factory test — system overlays may still preempt affect briefly; document in one place.

3. **Phase L assets reused**
   - `eve_expression_state`, gaze easing, idle micro-motions become **downstream of affect**, not upstream of behaviour.

---

## Target API (sketch)

```cpp
typedef struct {
  EveExpressionId expression;
  EveGazeDirection lookBias;
  bool blink;
  bool slowBlink;
  bool leftWink;
  bool rightWink;
} EveEyeAffectHint;

typedef struct {
  const char* wavPath;       /* SD path or nullptr */
  uint8_t volume;
  bool queue;
  bool stopPrevious;
} EveAudioAffectHint;

typedef struct {
  EveEyeAffectHint eyes;
  EveAudioAffectHint audio;
} EveAffectOutput;

void eveEmotionInit(void);
void eveEmotionApplyIntent(const EveBehaviourIntent* intent, uint32_t nowMs);
void eveEmotionGetAffect(EveAffectOutput* out);
void eveEmotionTick(uint32_t nowMs);   /* smooth blends, hold timers */
```

Eye Controller consumes eye hints (existing `eveEyeControllerApplyRequest` may evolve into this). Audio consumes audio hints via `audio_control` / asset manager.

---

## Example

```text
Behaviour:  Greeting
Intent:     wantGreeting, person close

Emotion:
  expression = Happy
  lookBias   = toward person zone
  blink      = single slow
  audio      = /audio/hello_02.wav

Outputs:
  Eye Controller → render happy + look + blink
  Audio Engine   → play hello_02.wav
```

---

## Migration from Phase L

| Phase L (today) | Phase Q |
|-----------------|---------|
| Emotion FSM BOOT→IDLE→CURIOUS… | Driven by Behaviour; Emotion holds **blend state** not **world FSM** |
| `eveEmotionOnTofSnapshot()` | Deleted; Behaviour reads Awareness |
| `eveExpressionSetOrchestrator()` from emotion states | From `eveEmotionApplyIntent()` |
| Voice widen in emotion | Audio hint + eye hint from Conversation behaviour |
| `eve_idle_engine` | Output polish or idle **hints** from Emotion; timing policy in Idle behaviour |

Keep:

- Expression catalog (Neutral, Happy, Curious, …)
- Gaze engine easing (called from Eye Controller tick)
- Voice **render** overlay (eyes only) as affect hint side-effect

---

## Tick order (after Phase Q)

```text
Awareness tick
Behaviour tick     → intent
Emotion tick       → affect (apply intent + smooth)
Eye Controller     → read affect / expression target
Audio tick         → read audio hint
```

---

## Success criteria

- [ ] No ToF/person transition logic in `eve_emotion_engine`
- [ ] Behaviour is sole intent source
- [ ] Audio hints routed without DFPlayer or direct behaviour → `audioPlayTrack` shortcuts
- [ ] Eye hints do not bypass Eye Controller
- [ ] Phase L bench keys still work where applicable
- [ ] All firmware targets compile

---

## Out of scope (Phase Q)

- New behaviours (Phase P follow-up)
- Micro-saccade tuning (Phase R)
- New expressions beyond existing catalog
