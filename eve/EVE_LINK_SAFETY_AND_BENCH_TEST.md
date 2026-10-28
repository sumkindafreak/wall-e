# EVE — link, stop latch, outputs and bench test

This page describes how EVE's firmware behaves after the link/safety/driver rework, what each
setting in `arduino_ide/EVE_Companion/config.h` does, and how to bench-test it on the real robot.

> **What has been tested and what hasn't.** The logic described here compiles for the ESP32-S3,
> and simulated host tests (`tests/eve_host/run_tests.sh`) check it. Nothing here has been run
> on EVE's hardware yet. The bench test at the end is how to do that.

---

## 1. One source tree

PlatformIO (`eve/platformio.ini`) and the Arduino IDE both build `eve/arduino_ide/EVE_Companion/`.
The old `eve/src` + `eve/include` copy is gone, so the two builds can no longer drift apart. The
wire protocol is shared with WALL-E's base through `firmware_common/include/eve_link_protocol.h`,
and `scripts/verify_protocol_headers.sh` checks the dock's copy of the IDs against it.

| Module | What it does | Runs on a PC? |
|---|---|---|
| `eve_control_core` | Handshake, heartbeats, link supervision, command validation, modes | yes |
| `eve_safety` | Latched stop, fault register, battery motion inhibit | yes |
| `eve_routines` | Timed, cancellable expressions (wake, lost, greet, scan…) | yes |
| `eve_battery_logic` | Plausibility, filtering, hysteresis, charging/full inference | yes |
| `eve_pin_check` | Boot-time GPIO conflict check | yes |
| `state_machine.cpp` | Arduino glue: connects the above to eyes, glow, audio, servos | no |
| `servo_control`, `neopixel_control`, `audio_control`, `battery_monitor` | Drivers | no |

## 2. The UART link to WALL-E

```
EVE                                   WALL-E base
EVE_HELLO {"proto":1,"caps":31,...} →
                                     ← WALL_E_ACK {"session":S,"peer":"wall_e","hb":250}
EVE_READY {"session":S}             →
EVE_HEARTBEAT (status, 4×/s)        →  ← WALL_E_HEARTBEAT {"session":S} (4×/s)
```

* **Only valid frames count.** A frame must pass CRC-16 and the version check, travel in the
  right direction (WALL-E → EVE), and carry the current session if it names one. Line noise,
  malformed frames, an old session and EVE's own frames echoed back can never keep the link alive.
* **Supervision.** With this WALL-E firmware (it sends `"hb"` in its ACK) EVE drops the link after
  **2 s** without a valid frame, **in every mode**. With an older peer that sends no heartbeats
  (older WALL-E firmware, the EVE dock) EVE keeps the original rule: 8 s in ESCORT, INTERACT and
  DOCKED.
* **WALL-E side** (`main_wall_E_base/main/eve_peer_link.cpp`) applies the same rules in reverse.
  If WALL-E restarts while EVE is still linked, WALL-E takes over EVE's running session. If EVE
  restarts, she sends HELLO and gets a new session.
* **"Linked" means talking, not touching.** A working UART link shows the boards are
  communicating. Physical contact is reported only when a presence pin is wired
  (`EVE_PRESENT_PIN`). Without one, contact shows as *unknown* in EVE's status and on WALL-E's page.

### Message IDs added (backward compatible)

| ID | Name | Direction | Payload |
|---|---|---|---|
| 0x0A | `MSG_EVE_CMD_RESULT` | EVE → WALL-E | `{"seq":N,"type":N,"ok":bool,"err":"reason","st":"MODE","stop":bool}` |
| 0x11 | `MSG_WALL_E_HEARTBEAT` | WALL-E → EVE | `{"session":S}` |
| 0x23 | `MSG_MODE_INTERACT` | WALL-E → EVE | `{"session":S}` |
| 0x24 | `MSG_MODE_STANDBY` | WALL-E → EVE | `{"session":S}` |
| 0x36 | `MSG_CLEAR_STOP` | WALL-E → EVE | `{"session":S}` (**session required**) |
| 0x37 | `MSG_RUN_ROUTINE` | WALL-E → EVE | `{"r":"greet"|"scan"|"play"|"protect"|"wake"}` |

No existing ID changed. Older firmware ignores the new ones.

### Command rules on EVE

Every command from 0x10 to 0x3F gets a `MSG_EVE_CMD_RESULT` reply. Refusals carry a reason.

| Command | Needs link | While **stopped** | Validation |
|---|---|---|---|
| `STOP` | **no** (accepted in every state, any or no session) | re-asserts the halt | — |
| `CLEAR_STOP` | yes + current session | clears | `session` must be present and match |
| `RESET_STATE` | yes | allowed; **stop stays latched** | — |
| `ATTACH_CONFIRMED`, `MODE_IDLE/ESCORT/INTERACT` | yes | refused `stopped` | ESCORT/INTERACT refused `low_power` on critical battery |
| `MODE_DOCK`, `MODE_STANDBY` | yes | allowed (calmer modes) | — |
| `MOVE_SERVO {"l","r","dps"}` | yes | refused | integers only, −360…360, clamped to calibration (`"clamped":true`); a missing channel is left alone |
| `PLAY_SOUND {"track"}` | yes | refused (track 0 = stop audio, always allowed) | 0…255 |
| `SET_EYES {"e","ms"}` or `{"mode"}` | yes | refused | known name, `ms` 0…30000; legacy mode 0…2 |
| `SET_GLOW {"p"}` or `{"pattern"}` | yes | refused | known name or 0…15, `"auto"` releases |
| `RUN_ROUTINE {"r"}` | yes | refused | operator routines only |

## 3. Stop, faults and link loss

* **STOP sources:** WALL-E (`MSG_STOP`; WALL-E sends it for the CYD E-STOP, CYD STOP_ALL, the LROS
  Emergency Stop and the EVE page), the USB serial console (`stop`), and EVE's docked web page.
* **A STOP:** cancels the active and pending gesture steps, halts the servos, stops audio
  (`audioStop()`), and shows the steady dim-red STOPPED glow. Routines, sound and motion are then
  refused **in the drivers themselves**, so no other code path can get around it.
* **It stays latched** until an explicit clear: `MSG_CLEAR_STOP` with the current session, the
  serial `clearstop` command, or *Clear Stop* on a web page. Reconnecting, `RESET_STATE`,
  `stateMachineInit()` and mode commands never clear it. **After a clear nothing resumes by
  itself.** Motion needs a new command. The docked desk-companion and spatial-awareness idle
  behaviours do start again, because they are new behaviour rather than a resumed gesture.
* **Hold or release:** `EVE_SERVO_STOP_POLICY`. The default is **HOLD**: pulses continue at the
  last commanded angle, so the servos keep their torque. RELEASE stops the pulses, and most
  hobby servos then go limp. Use RELEASE only if you have confirmed the arm and head cannot drop
  or slam.
* **Commanded vs physical position:** there is no position feedback. EVE reports the *commanded*
  angle; `-1` means nothing has been commanded since boot or since a RELEASE stop, so the real
  position is unknown. No pulse is sent at boot. The first move starts from the configured rest
  angle (a known pose) and eases from there.
* **Link loss:** cancel gestures, freeze the servos where they were last commanded (holding), drop
  any operator glow, play the *lost* expression once (no motion), then go back to waiting for
  WALL-E. Reconnecting plays *wake* once and starts in IDLE. Escort and other modes are **not**
  resumed.
* **Faults:** `PIN_CONFLICT` and `SERVO_DRIVER` are **blocking**: EVE enters ERROR mode, halts
  motion, keeps talking to WALL-E and reports the fault. `BATTERY_SENSOR`, `AUDIO` and `EYES` are
  **non-blocking**: EVE carries on without that part and reports the fault.

## 4. Modes and expressions

Mode priority: ERROR (blocking fault) > WAIT_ATTACH (no link) > DOCKED > LOW_POWER (verified
critical battery) > the mode WALL-E asked for (IDLE / ATTACHED / ESCORT / INTERACT / SLEEP).

| Event | Routine (once per event) | Notes |
|---|---|---|
| Waiting for WALL-E | — | slow dim blue pulse, *searching* eyes |
| Link established / leaving standby | **wake** | fade in, blink, soften, then affection: the warmer response to WALL-E |
| Link lost | **lost** | confused, then searching. No motion |
| ESCORT | **escort** | quick, precise, steady cool-blue glow |
| INTERACT | **greet** | pause, soften, small arm greet (132°), affection |
| STANDBY (SLEEP) | **standby** | slow settle, eyes close, green beacon. Heartbeats continue |
| Battery WARN (verified) | **low_warn** | once per descent; sensor glitches don't repeat it |
| Battery CRITICAL (verified) | **low_power** | motion inhibited; WALL-E is sent `EVE_LOW_BATTERY` every 20 s |
| Measured full | **full** | only when charging is actually measurable |
| Operator | **scan** / **play** / **protect** / **greet** / **wake** | *scan is an animation only: EVE looks left and right but detects nothing* |

The firmware does not claim object recognition, navigation, following or hovering. Proximity
glances (ToF) stay off until the sensors are confirmed installed and wired.

### Glow patterns (`SET_GLOW`, serial `glow`)

`0 idle` · `1 linked` · `2 waiting` · `3 critical` · `4 standby` · `5 interact` · `6 escort` ·
`7 low_power` · `8 charging` · `9 full` · `10 fault` · `11 stopped` · `12 scan` · `13 play` ·
`14 protect` · `15 off` · `auto`. Numbers 0, 1, 3, 4 and 5 keep their old meaning.

Layering: STOPPED > FAULT > routine overlay > operator choice (until the next mode change) > mode.
The onboard RGB LED (GPIO48) always shows it. The external strip shows it only when
`EVE_ENABLE_NEOPIXEL` is 1 and `EVE_NEOPIXEL_PIN` is set.

## 5. Sound track mapping

**This firmware ships no recordings.** Track numbers resolve through the SD card's
`/config/audio.json` (template in `sd_card_template/config/audio.json`), falling back to
`/audio/NNN.wav`. A missing or unreadable file is logged (`[EVE][ASSET] missing: …`), the request
returns "not played", and EVE carries on.

| Cue (config.h) | Track | Template file | Used by |
|---|---|---|---|
| `EVE_SND_WAKE`, `EVE_SND_GREET` | 1 | `hello.wav` | wake, greet |
| `EVE_SND_LOST`, `EVE_SND_SCAN`, `EVE_SND_PLAY` | 2 | `curious.wav` | lost, scan, play |
| — | 3 | `ack.wav` | remote `PLAY_SOUND 3` |
| `EVE_SND_STANDBY`, `EVE_SND_LOW_POWER` | 4 | `soft.wav` | standby, low power |
| `EVE_SND_PROTECT` | 5 | `alert.wav` | protect |
| — | 6 | `error.wav` | remote `PLAY_SOUND 6` |

Set any cue to `0` to keep that routine silent. Audio is the **SD card + I2S** engine already in
this branch. A DFPlayer is not configured anywhere in the current code, so no DFPlayer driver has
been added.

## 6. Battery

Configured sensor: **INA219** at 0x40 on SDA 8 / SCL 9, with the 2S Li-ion range already in
`config.h` (6.0–8.4 V, warn 25 %, critical 15 %). These thresholds are unchanged.

* **Pack voltage = bus + shunt.** The INA219 measures bus voltage at V−, after the shunt, so the
  shunt drop is added back. Every I2C read is checked. A shunt reading at the ±320 mV limit
  (about ±3.2 A on the 0.1 Ω breakout) marks the current as invalid.
* **Plausibility:** readings outside `EVE_BAT_PLAUSIBLE_MIN_V…MAX_V` (5.0–8.8 V by default) are
  **invalid**, not "empty" or "full". After three invalid samples in a row the level becomes
  *unknown* and the non-blocking `BATTERY_SENSOR` fault is set. An invalid reading never releases
  a low-power inhibit.
* **Filtering:** an exponential moving average over the 2 s polls rides through motor-current
  sags. Levels change with 4 % hysteresis.
* **Charging / full:** reported as **unknown** until `EVE_INA219_CHARGE_SIGN` says which way charge
  current reads. Set it after watching the `[EVE][BAT]` lines with the charger connected. "Full"
  needs charging, voltage near 8.4 V and the charge current tapering off.
* **Logging:** `[EVE][BAT]` on every level or charging change, otherwise every 30 s.
* **Legacy ADC path** (`EVE_BATTERY_INA219 0`, not used now): the 10k/10k divider would put 4.2 V
  on the ADC pin at full charge. The ESP32-S3 reads only 0–3.1 V at 12 dB attenuation, so that is
  out of range and above the pin's 3.3 V limit. The analog current-sensor constants match an
  ACS712-05B run at 3.3 V, below the ACS712's rated 4.5–5.5 V supply. Clipped readings are marked
  invalid. Don't switch to this path without changing the divider.

## 7. Configuration checklist (`config.h`)

| Part | Current build | To enable |
|---|---|---|
| UART to WALL-E | **on**, TX 17 / RX 18 | — |
| Onboard RGB LED | **on**, GPIO48 | — |
| INA219 battery | **on**, 0x40, SDA 8 / SCL 9 | set `EVE_INA219_CHARGE_SIGN` after measuring |
| Presence pin | off (`-1`) | `EVE_PRESENT_PIN`, `EVE_PRESENT_ACTIVE_LOW` |
| Eyes (ST7789 240×280 ×2) | off, pins `-1` | `EVE_ENABLE_EYES 1` + TFT pins; use a 3 MB app partition in the Arduino IDE |
| Servos (head pan, right arm) | off, pins `-1` | `EVE_ENABLE_SERVOS 1` + pins; check limits, rest, invert, pulse range, stop policy |
| NeoPixel strip | off, pin `-1` | `EVE_ENABLE_NEOPIXEL 1` + pin, count, type |
| SD + I2S audio | off, pins `-1` | `EVE_ENABLE_AUDIO 1` + SD and I2S pins |
| ToF (VL53L1X) | off, pins `-1` | `EVE_ENABLE_TOF 1` + I2C pins (the same as the INA219's: one Wire bus) + Pololu VL53L1X library |

The boot-time pin check blocks motion if two enabled drivers share a GPIO, or if a pin is
reserved on the N16R8 (flash 26–32, PSRAM 35–37) or doesn't exist.

## 8. Operator interfaces

**USB serial (115200):** `help`, `status`, `diag`, `stop`, `clearstop`, `standby on|off`,
`routine <name>`, `eyes <name> [ms]`, `glow <name|n|auto>`, `head <deg>`, `arm <deg>`,
`pose <h> <a>`, `audio <n>|stop`, `dock on|off`, `reset`, plus the existing personality and mic
commands.

**EVE's docked web page:** Safety card (STOP, Clear Stop, Standby / Wake) and a Routines row. The
old "Stop" pose preset now latches a real stop.

**WALL-E (LROS → EVE page):** *EVE state & safety* card. It shows the link and supervision mode,
contact (from the presence pin only), EVE's mode, stop, faults, routine, battery and the last
command verdict, with buttons for STOP, Clear stop, modes and routines. The LROS Emergency Stop and
the CYD E-STOP / STOP_ALL also latch EVE's stop. API: `/api/eve/stop` (no token, since a stop can
only make things safer), plus token-protected `/api/eve/clear_stop`, `/api/eve/mode?name=`,
`/api/eve/routine?name=`, `/api/eve/eyes?name=&ms=`, `/api/eve/glow?name=` and `/api/eve/reset`.
`/api/eve/status` keeps its old keys and adds `link`, `supervision`, `contact`, `eve{…}` and
`last_result`.

## 9. Bench test (existing build: UART + onboard LED + INA219)

Wheels off the ground, the arm clear of anything, USB serial open on **both** boards at 115200.

1. **EVE alone.** Power EVE with WALL-E unplugged. Expect `[EVE][CONFIG] … no conflicts`, the INA219
   line, `BOOT -> WAIT_ATTACH`, and the onboard LED in a slow dim blue pulse. `diag` should show
   `linked=no`, `stop=clear`, battery volts close to a multimeter reading across the pack.
2. **Battery sanity.** Compare `[EVE][BAT]` volts with the multimeter (they should agree within
   about 0.1 V). Plug in the charger: if the current sign flips, set `EVE_INA219_CHARGE_SIGN` to
   that sign, rebuild, and check that `charging=yes` appears.
3. **Handshake.** Connect the hand UART (crossover + common GND). EVE: `UART link up … supervision=
   heartbeat 2s`, `WAIT_ATTACH -> IDLE`, `start wake`, and the LED goes to brighter blue breathing.
   WALL-E: `linked with EVE`. The LROS EVE page should show **linked**, mode IDLE and contact
   *unknown*.
4. **Link loss.** Unplug the UART lead. Within about 2 s EVE logs `link down`, `start lost` (once)
   and goes back to the waiting pulse. WALL-E shows *waiting*. Reconnect: *wake* plays exactly
   once and EVE is in IDLE (not the earlier mode).
5. **Stop latch.** On the LROS EVE page press **STOP EVE**. EVE logs `STOP latched (src=wall_e)`
   and the LED goes steady dim red. Press *Escort* and *Greet*: "Last command" shows
   `refused: stopped`. Unplug and replug the UART: EVE is still stopped. Press *Clear EVE stop*:
   the LED returns to blue and nothing moves on its own.
6. **E-STOP path.** Press the CYD E-STOP. EVE latches again (`src` shows the stop came from
   WALL-E). Clear it from the EVE page.
7. **Modes.** Interact → violet glow and greet once. Escort → steady cool blue. Standby → green
   beacon, and the link stays up. Routine *wake* returns her to IDLE.
8. **Serial stop.** On EVE's USB console type `stop`, then `diag` (stop LATCHED, src serial), then
   `clearstop`.
9. **Only after confirming the servo pins and travel:** set them in `config.h` with the horns off
   first. Try `head 90`, `head 60` and `arm 100`, and check direction and limits. Then `stop` during
   a `routine play` and confirm the motion stops and holds, or releases if that is what you chose.

Record anything that differs from this. Those differences are the hardware facts the firmware
still needs.
