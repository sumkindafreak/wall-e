# Phase N — Dual physical eye displays and Eye Controller

## Architecture

```
Behaviour → Emotion Engine → Eye Controller → leftEye / rightEye → Display Drivers → SPI
```

Nothing outside **Eye Controller** draws to the left or right eye panels.

## Hardware

Shared SPI (SCK, MOSI, MISO) with separate chip selects:

| Device | CS pin (`config.h`) |
|--------|---------------------|
| Left eye LCD | `EVE_LEFT_EYE_CS` |
| Right eye LCD | `EVE_RIGHT_EYE_CS` |
| microSD | `EVE_SD_SPI_CS` |

When **both** eye CS pins are configured, `EVE_FACE_DUAL_PHYSICAL` is true: two independent LVGL displays (not mirrored).

## Modules

| File | Role |
|------|------|
| `eve_eye_controller` | Owns init/tick, leftEye + rightEye, sole render entry |
| `eve_eye` | Per-eye visual state (gaze, lids, scale, brightness) |
| `eve_eye_blink` | Independent blink timing per eye (offset, wink, slow, double) |
| `eve_single_eye_renderer` | Layer stack on one panel (background, glow, sclera, lids, highlight, scan) |
| `eve_eye_display_driver` | Dual SPI flush callbacks (left CS / right CS) |
| `eve_eye_renderer` | Legacy combined visor when only left CS is wired (dev/bench) |

## Emotion → eyes

Emotion engine does **not** draw. It updates expression/gaze/idle; Eye Controller reads `eveExpressionGetTarget()` and renders.

Optional requests: `eveEyeControllerApplyRequest()` (expression, look direction, blink flags).

## Blink policy

Idle and blink engine prefer **one eye at a time** for normal blinks; right eye schedule is phase-offset from the left.

## V2 roadmap

Eye subsystem is considered **mature** after this phase. Further eye work belongs in **Phase R** (expression polish) only. Brain stack: see `EVE_SOFTWARE_ARCHITECTURE.md` and Phases O–Q.
