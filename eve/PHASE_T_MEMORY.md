# Phase T — Memory (continuity)

**Goal:** EVE gains **continuity** — she remembers enough to feel like the same companion from one moment to the next, without bypassing Behaviour or replacing the deterministic stack.

This is **V4 — Memory**: *"What do I remember?"*

Depends on **Phase O** (Awareness), **Phase S** (Character). Memory feeds **Character only** — not Behaviour, Emotion, or Outputs directly.

---

## Four milestones (story)

| Milestone | Tagline | Question |
|-----------|---------|----------|
| **V1 — Body** | Can move and express | *How is EVE constructed?* |
| **V2 — Mind** | Can understand and decide | *How does EVE think?* |
| **V3 — Character** | Can feel unique | *Who is EVE?* |
| **V4 — Memory** | Can remember | *What happened before?* |

Each rung adds capability **without invalidating** the previous ones.

---

## Memory is context, not control

| | Character | Memory |
|---|-----------|--------|
| **Answers** | *Who am I?* | *What do I remember?* |
| **Horizon** | Traits, mood, preferences | Events, counts, recency |
| **Decides?** | No — biases via Character | **No** — provides context to Character |

Memory does **not** choose Greeting vs Idle. It might tell Character: *"I greeted this person 30 seconds ago"* → Character biases Behaviour toward a warm smile instead of repeating hello.

```text
Yesterday:
  Person approaches → Greeting → positive interaction → Memory stored

Tomorrow:
  Person approaches → Memory recalled → Character bias (recognised) →
  Behaviour (warmer greet path) → Emotion → Outputs
  (brighter eyes sooner, different hello WAV)
```

---

## Position in the pipeline

Not a parallel output layer — **context between Awareness and Character**:

```text
Hardware
    ↓
Awareness          (what is true now)
    ↓
Memory             (what was true before — context only)
    ↓
Character          (Personality + Mood + memory-informed bias)
    ↓
Behaviour
    ↓
Emotion
    ↓
Outputs
```

---

## Lightweight by design

No database. No LLM required. SD card under `/memory` or entries in `/config` (Phase M asset layout).

### Session / lifetime stats (example)

```json
{
  "timesStarted": 53,
  "lastWake": "2026-07-31T08:12:00Z",
  "lastGreeting": "2026-07-31T09:04:22Z",
  "knownPeople": 2,
  "lastInteractionDurationSec": 142,
  "favouriteGreeting": "hello03.wav"
}
```

### Event records (minimal v1)

```cpp
typedef enum {
  EVE_MEMORY_GREETING = 0,
  EVE_MEMORY_POSITIVE_INTERACTION,
  EVE_MEMORY_PERSON_SEEN,
  EVE_MEMORY_SLEEP,
  /* … */
} EveMemoryEventType;

typedef struct {
  EveMemoryEventType type;
  uint32_t timestampMs;
  float confidence;      /* 0..1 fusion quality when stored */
  uint32_t expiresMs;    /* 0 = use default TTL for type */
} EveRememberedEvent;
```

Events **expire** — EVE remembers for a while, not forever. Reduces SD wear and avoids stale social logic.

---

## Example experience (no hardware change)

Battery low. Person walks in.

**No memory:**

```text
Hello!   (full greeting every time)
```

**With memory:**

```text
…wakes…
looks over…
(recalls greeting 30 s ago)
…does not greet again…
just smiles.
```

Same eyes. Same audio engine. **Continuity** changes the experience.

Character applies the bias; Behaviour still owns the transition; Emotion still maps affect.

---

## API sketch

Memory is read by **Character only**.

```cpp
typedef struct {
  uint32_t lastGreetingAgeMs;   /* UINT32_MAX if never */
  uint32_t lastInteractionSec;
  uint8_t knownPeopleCount;
  bool recentPositiveInteraction;
  const char* preferredGreetingWav;  /* or nullptr */
} EveMemoryContext;

const EveMemoryContext& eveMemoryGetContext(void);

void eveMemoryTick(const EveAwarenessSnapshot* snap, uint32_t nowMs);
void eveMemoryRecord(EveRememberedEvent event);
void eveMemoryExpire(uint32_t nowMs);
bool eveMemoryLoadFromSd(void);
bool eveMemorySaveToSd(void);   /* debounced, not every tick */
```

Forbidden: `eveMemoryGetContext()` from Behaviour, Emotion, Eye Controller, or Audio.

---

## What Memory records (initial scope)

- Last greeting time / type
- Last wake / sleep
- Interaction duration buckets
- Optional: coarse person slot (not face recognition — proximity + time correlation until vision exists)
- Boot count, favourite cue paths (learned from repeated successful interactions)

All writes **append or replace small JSON** on SD — no SQL, no cloud.

---

## AI / LLM (explicit non-goal for Phase T)

The stack stays **deterministic, predictable, debuggable**.

If experimenting later:

- Speech understanding → **Awareness** source (facts)
- Optional language model → **advisor** suggesting intent, **never** replacing Behaviour Manager

Do not embed LLM inside Behaviour for core companion logic.

---

## Success criteria

- [ ] Repeat greeting suppressed when `lastGreetingAgeMs` below threshold (configurable)
- [ ] Recognised-return interaction feels warmer via Character bias, not hard-coded Emotion
- [ ] Events expire correctly; SD file size bounded
- [ ] Memory → Character only; no upward/forbidden deps
- [ ] All firmware targets compile

---

## Out of scope

- Face recognition / person IDs (future Awareness input may feed Memory)
- ChatGPT-style conversational memory
- Behaviour or Emotion reading SD directly
