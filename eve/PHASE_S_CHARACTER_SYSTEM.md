# Phase S — Character System

**Goal:** After Phases O–R, EVE gains **character** — who she is, not just what she does. Two identical robots on the same firmware can feel like completely different companions because of SD-loaded character profiles.

This is **V3 — Character**: *"Who is EVE?"* (Memory, V4, feeds context **into** Character — see `PHASE_T_MEMORY.md`.)

Depends on **Phase P** (behaviours) and **Phase Q** (emotion as affect only). Does not change layer boundaries or Eye Controller ownership.

---

## Four milestones

| Milestone | Tagline | Question |
|-----------|---------|----------|
| **V1 — Body** | Can move and express | *How is EVE constructed?* |
| **V2 — Mind** | Can understand and decide | *How does EVE think?* |
| **V3 — Character** | Can feel unique | *Who is EVE?* |
| **V4 — Memory** | Can remember | *What do I remember?* |

Through V1: **robot**. Phases O–P: **behaviour**. Phase S: **presence**. Phase T: **continuity**.

---

## Character System components

Phase S is the umbrella **Character System**. It does not replace Emotion; it **feeds Behaviour** before Emotion maps intent to affect.

```text
Awareness (facts now)
        │
        ▼
   Memory (Phase T)     ← context: last greet, recency, session stats
        │
        ▼
┌───────────────────┐
│  Character System │
│  · Personality    │  long-term traits (SD profile)
│  · Mood           │  short-term state (battery, activity, …)
│  · Preferences    │  optional: likes/dislikes (future)
└───────────────────┘
        │  biases thresholds & timings (informed by memory)
        ▼
   Behaviour
        ▼
   Emotion
        ▼
   Outputs
```

### Personality (long-term)

Stable traits loaded from SD. Same behaviour **classes**; different **thresholds and timings**.

**Do not** implement as hard rules like `if curiosity > 0.8 then Curious`. **Do** bias internal scores and timers.

Example — interest biasing:

```text
Awareness: motion detected
Behaviour computes:  Interest = 0.62
Personality (high curiosity):  effectiveInterest = 0.81
→ crosses Investigate threshold → Curious behaviour
```

Example — greeting delay:

```text
Base greet delay:     1.8 s
Shy personality:      3.6 s   (multiplier on delay, not a new behaviour)
```

Example — idle glances:

```text
Base idle glance:     8–12 s
Curious personality:  3–6 s   (interval range scaled)
```

The behaviour **name** does not change — its parameters do.

### Mood (short-term)

Temporary influence derived from **Awareness**, combined with personality before Behaviour decides.

| Example | Mood effect |
|---------|-------------|
| Personality: curious; battery low | Still curious, but slower to investigate, sleepier expression bias |
| Personality: playful; just woken | Extra playful until mood decays |
| Long idle | Mood drifts toward sleepy without changing SD personality |

Mood is **not** Emotion. Emotion still maps behaviour → affect for eyes and audio. Mood adjusts how personality expresses through Behaviour thresholds (and optionally soft hints to Emotion — document-only until Phase Q is stable).

```text
Personality: naturally curious (long-term)
Mood:        currently tired (short-term, battery low)
→ Behaviour: still investigates, but hesitates longer; Emotion may lean Sleepy overlay
```

### Preferences (future)

Configured or learned likes/dislikes (certain sounds, proximity comfort, WALL-E vs stranger). Stays inside Character System; feeds Behaviour biasing only.

---

## SD configuration

Prefer normalized **0..1** trait scales plus derived timings in firmware:

```json
{
  "character": {
    "profile": "eve_default",
    "personality": {
      "curiosity": 0.90,
      "shyness": 0.15,
      "patience": 0.85,
      "playfulness": 0.70,
      "sleepiness": 0.20
    }
  }
}
```

versus a shy profile on the **same firmware**:

```json
{
  "character": {
    "profile": "eve_shy",
    "personality": {
      "curiosity": 0.30,
      "shyness": 0.80,
      "patience": 0.95,
      "playfulness": 0.10,
      "sleepiness": 0.75
    }
  }
}
```

Firmware maps traits → multipliers on base thresholds (greet delay, interest gain, glance intervals, sleep eligibility). No per-profile behaviour code forks.

Mood is **runtime**, not primary SD config — computed each tick from Awareness (and optional decay timers).

---

## API sketch

Character System is read by **Behaviour only** (same dependency rule as Personality).

```cpp
typedef struct {
  float curiosity;    /* 0..1 */
  float shyness;
  float patience;
  float playfulness;
  float sleepiness;
} EvePersonalityTraits;

typedef struct {
  float tiredness;    /* 0..1 short-term */
  float alertness;
  float socialWarmth; /* optional decay after interaction */
} EveMoodState;

typedef struct {
  EvePersonalityTraits personality;
  EveMoodState mood;
} EveCharacterState;

const EveCharacterState& eveCharacterGetState(void);

/** Behaviour internal: bias a raw score before threshold compare */
float eveCharacterBiasInterest(float rawInterest);
uint32_t eveCharacterGreetDelayMs(uint32_t baseMs);
void eveCharacterTick(const EveAwarenessSnapshot* snap, uint32_t dtMs);
```

No Character APIs from Emotion, Eye Controller, or Audio.

---

## Success criteria

- [ ] Two SD profiles on one firmware build behave noticeably different in bench
- [ ] Personality **biases** decisions (no `if trait > x` behaviour forks)
- [ ] Mood shifts with battery / idle / activity without rewriting personality
- [ ] No upward dependencies (see `EVE_SOFTWARE_ARCHITECTURE.md`)
- [ ] All firmware targets compile

---

## Out of scope

- LLM / dialogue (future Behaviour input)
- New expressions (Phase R)
- Emotion owning world logic (Phase Q first)
- Architecture layer changes — **foundation stays stable**
