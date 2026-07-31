# Milestone 1.0 — Foundation complete

**Date:** 2026-07-31  
**Status:** Architecture complete. Next work creates **moments**, not systems.

The robot is not finished. The **foundation** is.

---

## What 1.0 means

Not a feature-complete EVE — a **contract**:

- Every future feature has a **place to live**
- Every contributor has a **decision framework** (`EVE_VISION.md`, `EVE_SOFTWARE_ARCHITECTURE.md`)
- Every subsystem has a **single responsibility**
- Layer stack is **locked**; implementation may refine it only when reality proves it wrong — not because a new idea sounds better

### Developmental arc (documented)

```text
V1 Body  →  V2 Mind  →  V3 Character  →  V4 Memory
```

See `EVE_VISION.md` for vision and PR filters.

---

## Project status (one line)

> **Architecture is complete. The next milestone is about creating moments.**

Central questions now:

- What does she **notice**?
- What does she **remember**?
- What kind of **companion** is she?

Not: pin assignments, LVGL flush callbacks, or another layer diagram.

---

## Next: Phase O — Awareness

**Objective:** Enable EVE to **notice the world without interpreting it.** Nothing more. Nothing less.

**Spec:** `PHASE_O_AWARENESS_LAYER.md`

### Success criteria (modest)

- [ ] Hardware reports facts
- [ ] Facts become one immutable `EveAwarenessSnapshot`
- [ ] Snapshot visible on Serial (bench)
- [ ] **No** behaviour changes
- [ ] **No** emotion changes
- [ ] **No** eye changes
- [ ] **No** audio changes

When that passes, the contract is **proven**.

### Recommended PR breakdown (small, reviewable)

| PR | Scope |
|----|--------|
| **O-1** | `EveAwarenessSnapshot` struct, getter, serial dump |
| **O-2** | ToF → snapshot publisher |
| **O-3** | Battery → snapshot publisher |
| **O-4** | Dock → snapshot publisher |
| **O-5** | UART / WALL-E link → snapshot publisher |

Each PR reviewable in isolation. No “implement Awareness” monolith.

---

## Guardrails (post-1.0)

**Architecture change policy:**

> The architecture may only change if **implementation proves it wrong** — not because a new idea sounds better.

**Feature filter:**

> Does this make EVE feel more like **herself**? (coherent behaviour, not capability checkbox)

**Layer filter:**

> Which layer owns the **decision**?

---

## References

| Doc | Purpose |
|-----|---------|
| `EVE_VISION.md` | Why — companion coherence, believability, timescales |
| `EVE_SOFTWARE_ARCHITECTURE.md` | How — layers, APIs, phases O–T |
| `PHASE_O_AWARENESS_LAYER.md` | Phase O implementation detail |
