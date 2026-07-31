# Milestone 1.1 — Awareness complete

**Target:** Phase **O-5** merged.

Not about lines of code — about what EVE **knows** before Behaviour asks *what should I do?*

---

## At Milestone 1.1, Awareness publishes

| Truth | Phase |
|-------|-------|
| Who am I? (uptime, health stubs) | O-1 |
| Who is near me? (person facts) | O-2 |
| How am I feeling physically? (battery) | O-3 |
| What am I connected to? (dock, WALL-E, remote) | O-4 |
| What is operational? (voice, audio, SD, display, audio ready) | O-5 |

All **present-tense facts**. Append-only snapshot. No decisions.

---

## What unlocks next

> **Given everything I know right now… what should I choose to do?**

That question moves to **Behaviour** — starting with **P-1 Observe** (`IdleBehaviour`).

Awareness is then **frozen for purpose** (can still append new facts, not rename/overload).

---

## Acceptance

Same contract-testing discipline as O-1…O-5: **the script is the spec**; robot does nothing.

See `PHASE_O5_AWARENESS_COMPLETE.md` for O-5 closure checklist.
