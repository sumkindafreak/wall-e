# Phase O — Awareness layer

**Goal:** One place for **facts** about the world and EVE’s own state. No intent, no emotion, no rendering, no audio policy.

Awareness publishes **representations of facts**, not devices. Behaviour thinks in Person, Energy, Connection — not ToF, INA219, or UART.

---

## Three truths (sub-milestones)

| Step | Question | Domain | Status |
|------|----------|--------|--------|
| **O-1** | Who am I? | Internal state (health, uptime stubs) | **Frozen** |
| **O-2** | Who is near me? | External awareness (person / ToF) | **Frozen** |
| **O-3** | How am I feeling **physically**? | Physiology (battery — not emotion) | **Frozen** |
| **O-4** | Am I connected? | Dock + link facts | Planned |
| **O-5** | What else is true? | Voice, audio, subsystem health — **completes Awareness** | Planned |

After O-5, Awareness is **complete** for V2 Mind. Phase **P-1** (Observe) may begin.

See `PHASE_O2_AWARENESS_TOF.md`, `PHASE_O3_AWARENESS_BATTERY.md`, `PHASE_O4_AWARENESS_LINK.md`, `PHASE_O5_AWARENESS_COMPLETE.md`.

---

## Engineering rule: Awareness is append-only

Once a fact exists in `EveAwarenessSnapshot`:

- **Do not rename it**
- **Do not move it**
- **Do not overload it**

Need a new concept? **Add a new field** (and a dedicated publisher step if needed).

Old Behaviour code must still compile unchanged when Awareness grows.

---

## Architecture

```text
Hardware (ToF, INA219, UART, mic*, GPIO)
        │
        ▼
   Awareness tick  (one publisher per subsystem step)
        │
        ▼
EveAwarenessSnapshot  (immutable per frame)
        │
        ▼
   Behaviour Manager only (primary consumer — Phase P)
```

\* Microphone when hardware exists (O-5).

---

## Contract testing

Each O-step ships an **acceptance script** on the issue — watch serial, verify facts, robot does nothing. The script **is** the spec.

---

## Rules

- **No opinions** — no “curious”, “should greet”, or “go sleep” in Awareness.
- **Derived booleans** where fusion may grow (`personPresent`, `batteryLow`, …).
- **Copy-out** — `eveAwarenessGetSnapshot()` returns const; no mutable globals.

```cpp
void eveAwarenessInit(void);
void eveAwarenessTick(void);
const EveAwarenessSnapshot& eveAwarenessGetSnapshot(void);
```

---

## Tick order (target)

```text
eveBatteryTick()
… state / drivers …
tofTick()
eveAwarenessTick()   ← reads fresh battery + ToF frames
```

---

## Out of scope (Phase O)

- Behaviour FSM (Phase P — starts with **P-1 Observe**)
- Emotion refactor (Phase Q)
- Eye/audio policy (Phase R)
- SLAM, camera, ML

---

## Success criteria (Phase O complete = O-5 merged)

- [x] O-1 snapshot + serial bench
- [x] O-2 person facts (ToF only)
- [x] O-3 battery facts (five fields, derived low/charging)
- [ ] O-4 link/dock facts (connection, not hardware names)
- [ ] O-5 voice + subsystem health — Awareness layer **closed**
- [ ] No decision logic in Awareness at any step
- [ ] All firmware targets compile
