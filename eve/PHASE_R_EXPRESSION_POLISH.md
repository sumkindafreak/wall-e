# Phase R — Expression polish

**Goal:** Make EVE feel **alive** through subtle output refinement. No new behaviour rules, no Awareness changes, no Emotion world logic. Work stays **inside output layers** (primarily Eye Controller; secondary audio fades).

Depends on **Phase Q** (stable affect → output path).

---

## Scope

Display architecture is **frozen** after Phase N. Phase R only tunes:

- Micro-saccades
- Asynchronous blinking (left/right timing)
- Gaze easing curves and dwell before return-to-centre
- Idle output refinement (quiet pauses, tiny movements)
- Optional: audio crossfade / breath gaps between cues

---

## Eye Controller

| Technique | Intent |
|-----------|--------|
| **Micro-saccades** | Tiny involuntary gaze jitter; already partially via blink micro-gaze — increase subtlety and independence per eye |
| **Async blinks** | Right eye phase-offset from left; avoid simultaneous closes except rare “symmetric” beat |
| **Gaze easing** | No snapping; configurable ease for look-at-person and return-home |
| **Dwell timing** | Hold gaze on target before easing away (matches human attention) |
| **Idle refinement** | Longer random pauses; smaller amplitude movements |

Implementation homes:

- `eve_eye_blink` — timing, wink, double, random one-eye policy
- `eve_gaze_engine` — ease constants, dwell, yield to track
- `eve_idle_engine` or idle **output** scheduler — if still used post–Phase Q, only as hint consumer inside Eye Controller tick chain
- `eve_single_eye_renderer` — no logic change; visual only if needed

---

## Audio (light touch)

- Short fade-in/out on WAV start/stop
- Debounce rapid cue spam from Conversation behaviour
- No new playback architecture (Phase M baseline)

---

## Bench / validation

With `EVE_ENABLE_EYES=1`:

- Record short clips of idle + curious + greet sequences
- Compare blink overlap rate (target: mostly async)
- Serial metrics optional: blink phase offset, gaze settle time

---

## Success criteria

- [ ] Measurable async blink policy (document default offsets)
- [ ] Gaze dwell before return documented and tunable via `config.h` or SD config
- [ ] Micro-saccades visible but not distracting
- [ ] No changes to Awareness / Behaviour / Emotion contracts
- [ ] All firmware targets compile

---

## Out of scope (Phase R)

- New behaviours or sensors
- Dual-eye hardware changes
- Lip sync / mouth (EVE remains eyes-only for expression)
