# EVE — project vision

This is not a feature checklist. It is a model of **how a companion exists**.

---

## Vision statement

> **The goal of the WALL-E / EVE project is not to build an animatronic robot.**
>
> The goal is to build a companion whose behaviour feels **coherent over time**.
>
> Every subsystem exists to support that illusion:
>
> - **Awareness** notices the world.
> - **Memory** remembers it.
> - **Character** gives the robot individuality.
> - **Behaviour** decides what to do.
> - **Emotion** decides how to express it.
> - **Eyes and audio** communicate that expression.
>
> No layer bypasses another.
>
> **The illusion comes from consistency, not complexity.**

Technical architecture: `EVE_SOFTWARE_ARCHITECTURE.md`.

---

## Developmental milestones

Less a software roadmap, more stages of a living creature. Each builds on the last; none invalidates what came before.

```text
Body  →  Mind  →  Character  →  Memory
(V1)     (V2)     (V3)           (V4)
```

| Milestone | Tagline |
|-----------|---------|
| **V1 — Body** | Can move and express |
| **V2 — Mind** | Can understand and decide |
| **V3 — Character** | Can feel unique |
| **V4 — Memory** | Can remember |

The project began with **hardware**, matured into **software architecture**, and now addresses **interaction psychology** — not clinical psychology, but the design of believable social presence.

---

## Design questions (not engineering questions)

Many open problems are now **design** questions:

- How long should she maintain eye contact?
- How long should she wait before greeting?
- When should she look away?
- How often should she blink while listening?
- How quickly should she investigate movement?
- How long before curiosity fades?

Answers live in Character parameters, Behaviour thresholds, and Phase R output polish — tuned for **presence**, not new subsystems.

---

## Three timescales

Every layer operates on a different **clock**. This ties the stack together.

### Milliseconds — Outputs

Rendering, blink timing, audio playback, eye easing, micro-saccades.

**Layers:** Eye Controller, Audio Engine (output refinement inside Emotion hints).

### Seconds — Mind

Behaviour transitions: greeting, looking, following, conversation, investigate vs idle.

**Layers:** Awareness (facts each tick), Behaviour, Emotion (affect for this moment).

### Hours / days — Identity & continuity

Memory, character profile, preferences, session continuity across boots.

**Layers:** Memory, Character (personality + mood decay), SD-backed config.

```text
Milliseconds     Seconds              Hours / Days
────────────     ───────              ────────────
Outputs          Behaviour            Memory
(blink, ease)    Emotion (moment)     Character
                 Awareness (now)
```

Do not implement long-term memory in the blink engine, or greeting policy in the lid renderer.

---

## One question for every contributor

Before touching code:

> **When adding a new feature, first decide which layer owns the *decision* — not which file should contain the code.**

That single question forces good architecture.

| Situation | Owns the decision |
|-----------|-------------------|
| Battery low (fact) | **Awareness** |
| Robot looks tired (expression) | **Emotion** |
| Robot chooses to dock (intent) | **Behaviour** |
| Robot remembers docking yesterday | **Memory** |
| Shy robot waits longer before greet | **Character** (biases Behaviour) |
| Eyelid closes for blink | **Outputs** (Eye Controller) |

Everything else follows.

---

## What success feels like

Hardware fades into the background. People interacting with EVE feel like they are interacting with **her** — not a board, not a demo, not a feature list.

Consistency across time scales creates that illusion. Complexity does not.

---

## Going forward (architecture complete)

**Architectural saturation reached.** Documentation now serves the project; do not chase it with more layer diagrams or redesigns.

### Believability over capability

> **Every new capability must make EVE feel more believable, not merely more capable.**

Those are not the same thing.

- Facial recognition — only if it improves **continuity**
- Speech recognition — only if it improves **conversation**
- More sensors — only if they lead to more **natural behaviour**
- More animations — only if they improve **expression**

Complexity that does not strengthen the illusion is probably not worth adding.

### One question per PR

> **Does this make EVE feel more like herself?**

Not: *Is it technically clever?* *Does it use more AI?* *Does it add another checkbox?*

Instead: *Does this make her behaviour more coherent?*

People forgive limited capabilities when a robot is **internally consistent**. They are less forgiving of impressive tech that behaves unpredictably.

### Next implementation step

**Phase O — Awareness.** Not the flashiest subsystem, but the first place the architecture becomes *real*. Once `EveAwarenessSnapshot` exists, everything above evolves independently as long as that contract stays stable.

From here the question is not *"How should EVE be built?"* but **"What should EVE experience next?"**

