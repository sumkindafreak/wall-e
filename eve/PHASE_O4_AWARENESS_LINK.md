# Phase O-4 — Dock & link → Awareness (connection facts)

**Goal:** EVE knows **connection state** — not which GPIO or UART frame type. Same discipline as O-2/O-3: facts in, nothing reacts.

O-1/O-2/O-3 remain **frozen**.

---

## Think facts, not devices

| Device thinking (avoid) | Fact thinking (Awareness) |
|-------------------------|---------------------------|
| `dockConnected` | `docked` — is EVE on her dock? |
| UART session active | `wallELinked` — paired with WALL-E? |
| Companion port open | `remoteConnected` — remote operator path live? |

**Note:** `charging` is **physiology** (O-3 / battery). O-4 does not republish it.

---

## Inputs

**Dock + UART link subsystems only** (`state_machine`, `uart_link` — read-only facts).

No ToF, battery, emotion, eyes, or behaviour in `publishLinkFacts()` / `publishDockFacts()`.

---

## Outputs (connection block)

| Field | Meaning |
|-------|---------|
| `docked` | EVE is physically on dock (fact) |
| `wallELinked` | Active WALL-E hand session / peer linked |
| `remoteConnected` | Companion / remote UART path allowed |

Exactly **three** connection facts for O-4. No fourth field “because UART might be useful.”

*(Field name `docked` is frozen — append-only. Semantically: “is docked”.)*

---

## Acceptance test (bench)

Watch serial `AWARENESS` only. Robot does **absolutely nothing**.

```text
Place on dock (or docktest on).
↓
docked = true
↓
Remove from dock.
↓
docked = false
↓
Connect WALL-E UART / establish session.
↓
wallELinked = true
↓
Disconnect / link lost.
↓
wallELinked = false
↓
Robot does absolutely nothing.
```

Success.

---

## Out of scope (O-4)

- Charging (O-3)
- Person / battery fields
- Behaviour reactions to dock (Phase P+)
- WebUI AP policy (output side effect — not Awareness)

---

## Success criteria

- [ ] `publishDockFacts()` + `publishLinkFacts()` read link/dock modules only
- [ ] Three connection outputs; no new unrelated fields
- [ ] No behaviour/emotion/eye/audio changes
- [ ] All firmware targets compile
