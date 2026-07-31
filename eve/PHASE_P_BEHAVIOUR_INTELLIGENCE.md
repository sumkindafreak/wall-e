# Phase P — Behaviour intelligence

**Goal:** EVE’s **personality and intent** live here. One active **behaviour** at a time; each implements `enter` / `update` / `exit`. Behaviour consumes `EveAwarenessSnapshot` and produces **intent**, not graphics or WAV paths.

Depends on **Phase O** (Awareness snapshot).

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
 Idle    Curious    Follow    Greeting …
   │         │         │          │
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
| **Idle** | Default, timeout from others | Rest, ambient scan |
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
| `eve_idle_engine` emotion coupling | Idle behaviour owns idle timing policy |
| `eveBehaviorOnRemoteSound()` | Conversation or Greeting behaviour entry |
| `eve_shared_behaviour` | Awareness + Behaviour (link facts vs intent) |

Emotion FSM may still run in parallel during migration; goal is Behaviour as **sole** intent source by end of Phase P.

---

## Success criteria

- [ ] Behaviour Manager with one active behaviour
- [ ] Idle + Curious implemented and switching on snapshot
- [ ] Follow + Greeting + Sleep stubbed or partial
- [ ] No new rendering code in behaviours
- [ ] Documented transition table
- [ ] All firmware targets compile

---

## Out of scope (Phase P)

- Full Emotion decoupling (Phase Q)
- Eye micro-saccades (Phase R)
- New hardware drivers
