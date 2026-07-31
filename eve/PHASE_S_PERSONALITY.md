# Phase S — Personality

**Goal:** After Awareness, Behaviour, Emotion, and Outputs are stable (Phases O–R), give EVE **preferences** — not just reactions. Personality is what makes two physically identical robots feel like different companions.

Depends on **Phase P** (behaviours) and ideally **Phase Q** (emotion as affect only). Does not change layer boundaries or Eye Controller ownership.

---

## What personality is (and is not)

| Personality is | Personality is not |
|----------------|-------------------|
| Tunable thresholds and timings in Behaviour | New hardware |
| Hesitation before Curious, dwell on greet | Emotion deciding world state |
| Idle glance frequency, sleepiness rate | Display architecture changes |
| Shy vs bold parameter profiles | Hard-coded SPI or audio paths |

Personality **biases** Behaviour; it does not bypass the stack:

```text
Awareness → Behaviour (+ personality params) → Emotion → Outputs
```

---

## Example traits

| Trait | Effect |
|-------|--------|
| **Eye contact duration** | Follow behaviour hold time before look-away |
| **Investigate eagerness** | Delay before entering Curious vs immediate |
| **Sleepiness rate** | Idle minutes before Sleep behaviour eligible |
| **Greeting warmth** | Immediate Greeting vs pause + Curious first |
| **Idle restlessness** | Glance-around frequency in Idle behaviour |

These are **behavioural** parameters. Emotion still maps intent to affect; Eye Controller still renders; Audio still plays cues from hints.

---

## Configuration

Prefer **SD-driven** data under `/config` (Phase M asset system):

```json
{
  "personality": {
    "name": "default",
    "curiousDelayMs": 400,
    "greetDelayMs": 800,
    "eyeContactHoldMs": 3500,
    "idleGlanceIntervalMs": [8000, 18000],
    "sleepAfterIdleMs": 300000
  }
}
```

Fallback to `config.h` defaults when SD config absent.

---

## API sketch

Personality is read by **Behaviour** only:

```cpp
typedef struct {
  uint32_t curiousDelayMs;
  uint32_t greetDelayMs;
  uint32_t eyeContactHoldMs;
  uint32_t sleepAfterIdleMs;
  float idleGlanceRate;  /* 0..1 */
} EvePersonalityProfile;

const EvePersonalityProfile* evePersonalityGet(void);
bool evePersonalityLoadFromSd(void);
```

No `evePersonalityGet()` from Emotion or Eye Controller.

---

## Success criteria

- [ ] At least two named profiles (e.g. `default`, `shy`) behave noticeably different in bench
- [ ] Personality loaded from SD or compile-time default
- [ ] No new upward dependencies (see `EVE_SOFTWARE_ARCHITECTURE.md`)
- [ ] All firmware targets compile

---

## Out of scope

- LLM / dialogue (future Behaviour input, not Phase S)
- New expressions or eye layers (Phase R only)
- Emotion world logic (Phase Q must be done first)
