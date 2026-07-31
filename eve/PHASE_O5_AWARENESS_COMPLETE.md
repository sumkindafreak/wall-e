# Phase O-5 — Complete Awareness (voice, audio, health)

**Goal:** Close the Awareness layer. Remaining O-1 stubs become disciplined publishers. After O-5 merges, **Phase O is complete** and **P-1 Observe** may begin.

O-1 through O-4 remain **frozen** except append-only additions documented here.

---

## Inputs

| Publisher | Subsystem |
|-----------|-----------|
| `publishAudioFacts()` | `mic_input`, `audio_control` |
| `publishHealthFacts()` | `eve_asset_manager`, eye/audio ready flags |

No cross-subsystem reads inside each publisher.

---

## Outputs

### Audio / voice

| Field | Meaning |
|-------|---------|
| `voiceDetected` | Local mic level above threshold (fact) |
| `audioPlaying` | I2S output actively playing (fact) |

Not “someone is talking to me” (interpretation — Behaviour).

### Subsystem health

| Field | Meaning |
|-------|---------|
| `sdMounted` | SD assets available |
| `displayReady` | Eye/display stack ready |
| `audioReady` | Audio engine initialized |
| `uptimeMs` | Monotonic uptime since boot |

Health flags are **facts**, not “show error face.”

---

## Acceptance test (bench)

```text
Boot.
↓
uptimeMs increases
↓
sdMounted / displayReady / audioReady reflect reality
↓
Speak near mic (if wired).
↓
voiceDetected toggles
↓
Play test clip (bench).
↓
audioPlaying = true, then false
↓
Robot does absolutely nothing.
```

Success.

---

## After O-5 — Awareness complete

Awareness then holds the full V2 fact model:

```text
Person      (O-2)
Energy      (O-3)
Connection  (O-4)
Voice/health (O-5)
+ O-1 internal timing
```

Nothing interprets. Nothing reacts. **Behaviour (P-1)** gets its first meaningful input.

---

## Out of scope (O-5)

- Behaviour, Emotion, outputs
- New snapshot fields beyond listed append-only completions
- Ambient dB / ML voice activity (future append if needed)

---

## Success criteria

- [ ] Each publisher reads one subsystem family only
- [ ] Serial bench documents all fact groups
- [ ] Phase O checklist in `PHASE_O_AWARENESS_LAYER.md` complete
- [ ] No behaviour/emotion/eye/audio **policy** changes
- [ ] All firmware targets compile
