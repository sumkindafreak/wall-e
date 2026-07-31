# Phase P — Behaviour intelligence

**Goal:** EVE’s **personality and intent** live here. One active **behaviour** at a time; each implements `enter` / `update` / `exit`. Behaviour consumes `EveAwarenessSnapshot` and produces **intent**, not graphics or WAV paths.

Depends on **Phase O complete** (O-5 merged).

---

## P-1 — Observe (first behaviour)

**Not** Follow. **Not** Greeting. **Not** Conversation.

The first behaviour is **Observe** — EVE is aware of something interesting and **chooses** to do nothing visible yet.

```text
Awareness
↓
personPresent?
↓
Behaviour → Observe
↓
(no visible output)
```

That is agency: capable of reacting, deciding not to. Different from firmware that simply cannot respond.

| Design name | Code name (keep simple) |
|-------------|-------------------------|
| **Observe** | `IdleBehaviour` / `EVE_BEHAVIOUR_IDLE` |

Document intent as *observing*; implementation stays a minimal default behaviour.

### P-1 behaviour tree (fits on a napkin)

```text
personPresent == false  →  remain Observe
personPresent == true   →  remain Observe   (Curious does not exist yet)
```

Both paths are successful — **Behaviour** makes the decision, not Emotion or Eyes.

### Acceptance test (P-1)

Person walks into ToF range. Eyes unchanged. No greeting. No audio. Serial may log behaviour = Observe. **Decision** happened; output did not.

---

## Cognitive arc (after P-1)

Observe is the root of a process, not a dead state:

```text
Observe → Curious → Follow → Greeting → …
```

---

## Architecture

```text
EveAwarenessSnapshot
        │
        ▼
  Behaviour Manager
        │
   (one active)
        │
   ┌────┴────┬─────────┬──────────┐
   ▼         ▼         ▼          ▼
 Observe  Curious    Follow    Greeting …
 (Idle)       │         │          │
   └─────────┴─────────┴──────────┘
                    │
                    ▼
            EveBehaviourIntent
                    │
                    ▼
            Emotion (Phase Q)
```

---

## Behaviours (not “modules”)

Each behaviour is a struct of function pointers or a small class-style table:

```cpp
typedef struct {
  void (*enter)(const EveAwarenessSnapshot* snap);
  void (*update)(const EveAwarenessSnapshot* snap, uint32_t dtMs);
  void (*exit)(void);
  const char* name;
} EveBehaviour;
```

Planned set (initial):

| Behaviour | Typical entry | Intent |
|-----------|---------------|--------|
| **Observe** (`IdleBehaviour`) | Default; P-1 first behaviour | Aware, no visible action yet |
| **Curious** | Motion, new presence | Investigate stimulus |
| **Follow** | Sustained target in zone | Maintain interest, track |
| **Greeting** | Close + stable presence | Social opening |
| **Conversation** | Voice / remote sound / listen | Engage without losing track |
| **Sleep** | Dock, low battery, long idle | Low activity |

Naming note: “Investigate” in conversation maps to **Curious** in code.

Only one behaviour active. Transitions happen in **Behaviour Manager** based on snapshot + internal timers, not in Emotion.

---

## Behaviour Manager

```cpp
void eveBehaviourInit(void);
void eveBehaviourTick(uint32_t nowMs);

/* Optional: force behaviour for bench */
void eveBehaviourForce(const char* name);

EveBehaviourId eveBehaviourGetActive(void);
```

Responsibilities:

- Register all behaviours
- Choose transitions (priority table + hysteresis to avoid flicker)
- Call `exit` → `enter` on switch
- Emit **intent** each tick (see below)

**Current code:** Extend `eve_behavior_manager` → rename internally to match “Behaviour Manager” doc; today `eveBehaviorManagerTick()` is empty and remote sound bypasses intent — migrate in this phase.

---

## Intent (handoff to Emotion)

Behaviour does not call `eveExpressionRequest()` directly in the target design (Phase Q may allow a thin bridge during migration).

Target struct (sketch):

```cpp
typedef struct {
  EveBehaviourId behaviour;
  /* Hints for Emotion — not final affect */
  bool wantTrackPerson;
  bool wantGreeting;
  bool wantListen;
  float priority;
} EveBehaviourIntent;
```

Emotion maps e.g. `Curious` + `wantTrackPerson` → Curious expression + track gaze.

---

## Example flow

```text
Awareness: personPresent=true, distance=800mm, zone=CENTER
Behaviour:  Curious (entered 200ms ago)
Intent:     investigate + track
Emotion:    Curious (Phase Q)
Eyes:       curious shape, gaze center, random blink
Audio:      (none or soft idle loop — Phase Q hint)
```

---

## Migration from V1

| Today | Phase P |
|-------|---------|
| `eve_emotion_engine` ToF transitions | Move rules to Behaviour |
| `eve_idle_engine` emotion coupling | Observe (`IdleBehaviour`) owns idle timing policy |
| `eveBehaviorOnRemoteSound()` | Conversation or Greeting behaviour entry |
| `eve_shared_behaviour` | Awareness + Behaviour (link facts vs intent) |

Emotion FSM may still run in parallel during migration; goal is Behaviour as **sole** intent source by end of Phase P.

---

## Success criteria

- [ ] Behaviour Manager with one active behaviour
- [ ] **P-1 Observe** (`IdleBehaviour`) — reads snapshot, decides, no output
- [ ] Curious implemented and switching on snapshot (P-2+)
- [ ] Follow + Greeting + Sleep stubbed or partial
- [ ] No new rendering code in behaviours
- [ ] Documented transition table
- [ ] All firmware targets compile

---

## Out of scope (Phase P)

- Full Emotion decoupling (Phase Q)
- Eye micro-saccades (Phase R)
- New hardware drivers
