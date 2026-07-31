# Phase L — EVE expression, gaze, idle, and emotion engines

Eyes-only behaviour: **no mouth, lip sync, or jaw animation**. Voice and social cues use eye shape, lids, blink timing, and gaze.

## Modules

| File | Role |
|------|------|
| `eve_emotion_engine` | State machine: BOOT → IDLE → CURIOUS → FOLLOW → INTERACT → HAPPY → THINKING → SLEEP |
| `eve_gaze_engine` | Eased gaze (left/right/up/down, track, return centre); yields to head pan tracking |
| `eve_idle_engine` | Non-repetitive idle: blinks, double/slow blink, wink, glances, look-around, thinking/sleepy beats |
| `eve_expression_state` | Extended expressions + orchestrator + voice overlay |
| `eve_eye_animations` | Blink variants, wink (per-eye lids), squint/widen overlays |

## Emotion ↔ ToF

1. Person detected → **CURIOUS**, gaze toward zone  
2. Person close → brief **SURPRISED** + eye widen  
3. Dwell → **ENGAGED** → **HAPPY**  
4. Person leaves → watch last zone → timeout → **IDLE**

Pipeline: `eve_spatial_awareness` → `eveEmotionOnTofSnapshot()`.

## Voice

`eveBehaviorOnRemoteSound()` → `eveEmotionNotifyVoice()`: slightly wider eyes, softer lids, occasional blink. `eveExpressionSetVoiceActive()` applies a render overlay.

## Tick order (face)

`eveEmotionTick` → `eveGazeTick` → `eveExpressionTick` → `eveIdleTick` → `eveEyeAnimationsTick` → LVGL apply.

## Priority (expressions)

Critical/system states (battery, record fail, search, track, dock, WALL-E disconnect) still override the emotion orchestrator.

## Bench

With `EVE_ENABLE_EYES=1` and `EVE_FACE_DEBUG_BENCH=1`, serial keys in Phase J still apply; emotion logs `[EVE_EMOTION] State -> …`.
