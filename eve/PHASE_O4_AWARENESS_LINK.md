# Phase O-4 — Connection → Awareness (link facts)

**Status: frozen when merged.** Exactly three connection facts — present tense only.

**Goal:** EVE knows **"What am I connected to?"** — not what to do because of it.

Present-tense facts only. No `wallERecentlyDisconnected` (that is Memory, Phase T).

---

## Inputs

**Connection subsystems only** (`state_machine` — dock + UART session facts).

No ToF, battery, behaviour, emotion, eyes, or audio in `publishConnectionFacts()`.

---

## Outputs (exactly three)

| Field | Meaning (now) |
|-------|----------------|
| `docked` | EVE is in docked state |
| `wallELinked` | Active WALL-E session with linked peer |
| `remoteConnected` | Companion / remote UART path currently allowed |

Nothing else. No inferred emotions. No docking decisions. No greeting logic.

---

## Derived rule

```cpp
wallELinked = (sessionId != 0) && peerLabelIsLinked(peerLabel);
remoteConnected = stateMachineAllowsCompanionUart(); /* present fact from SM */
docked = stateMachineIsDocked();
```

---

## Tick order

`uartLinkPoll()` → `stateMachineTick()` → … → `eveAwarenessTick()`.

---

## Acceptance test (bench)

Watch serial `AWARENESS` only. Robot does **absolutely nothing**.

If EVE smiles, plays a sound, or moves her eyes — **failure**.

```text
Boot robot
↓
Connect WALL-E
↓
wallELinked = YES
↓
Disconnect
↓
wallELinked = NO
↓
Connect remote
↓
remoteConnected = YES
↓
Disconnect
↓
remoteConnected = NO
↓
Place on dock (or: docktest on)
↓
docked = YES
↓
Remove from dock (or: docktest off)
↓
docked = NO
↓
Robot does absolutely nothing.
```

Success = serial output changes correctly.

Host helpers:

```bash
cd eve && g++ -std=c++11 -Wall -Wextra -I awareness \
  -o /tmp/awareness_o4_connection_test test/awareness_o4_connection_test.cpp && \
  /tmp/awareness_o4_connection_test
```

---

## Out of scope (O-4)

- Person / battery fields (O-2/O-3 frozen)
- Voice, SD, display (O-5)
- Behaviour, emotion, outputs

---

## Success criteria

- [x] `publishConnectionFacts()` reads `state_machine` only
- [x] Three connection outputs; no fourth connection field
- [x] No behaviour/emotion/eye/audio changes
- [x] All firmware targets compile
