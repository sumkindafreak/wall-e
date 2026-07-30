# AGENTS.md

## Cursor Cloud specific instructions

This repo is a **multi-node ESP32/ESP32-S3 firmware monorepo** (PlatformIO + Arduino
framework) plus one **static browser UI** (`webui/`, "LROS"). There is no database, no
Node/JS build step, and no automated test suite / CI. See `README.md`, `ARCHITECTURE.md`,
and `PLATFORMIO_QUICK_START.md` for the full picture.

### What can actually run vs only compile
- **`webui/` (LROS) is the only component that genuinely runs on this Linux VM.** It is
  plain static HTML/CSS/JS — no build, no dependencies. Serve it with any static server,
  e.g. `python3 -m http.server 8000` from inside `webui/`, then open
  `http://localhost:8000/index-standalone.html` (self-contained offline/demo variant that
  needs no robot/base backend). `index.html` is the live variant that expects a reachable
  base at an IP such as `192.168.4.1`.
- **All firmware modules are cross-compiled for ESP32-S3 hardware — they can be built
  (`pio run`) but never executed on this VM.** Flashing/running requires physical boards.

### Toolchain (installed by the update script)
- `pip install --user platformio` puts `pio`/`platformio` in `~/.local/bin`. That dir is
  already on PATH via `~/.profile` and `~/.bashrc`. If a non-login shell can't find `pio`,
  run `export PATH="$HOME/.local/bin:$PATH"` first.
- The first `pio run` for any module downloads the `espressif32` platform, xtensa/riscv
  toolchains, and per-module `lib_deps` into `~/.platformio` (needs network; cached after).

### Building firmware
- Build one module at a time from its folder, using its default env, e.g.
  `cd main_wall_E_base && pio run` (default env `wall_e_brain_s3`), `cd eve && pio run`
  (`eve_s3`), `cd dock_station && pio run` (`dock_esp32`), etc. Each module's `platformio.ini`
  declares its `default_envs`. `ota_build_all.sh` builds base + controller + dock in sequence.

### Known pre-existing build blockers on branch `feature/eve-lvgl-tof-assist-voicebox-stack`
These are **source/config issues in the repo, not environment problems** (a fresh, correct
PlatformIO install still hits them). Do not treat them as setup failures:
- **ESP32 Arduino core version mismatch.** Several modules call the core-3.x API
  `ledcAttach(pin, freq, resolution)` (e.g. `main_wall_E_base/main/display_manager.cpp`,
  `dock_ir_transmitters.cpp`, `motor_control.cpp`, `laser_control.cpp`,
  `ghostbusters_slime_blower/*.ino`). The bare `platform = espressif32` in every
  `platformio.ini` resolves to the official platform shipping **Arduino core 2.0.17**, where
  that 3-arg overload does not exist (only `ledcSetup`/`ledcAttachPin`). Building against a
  **core-3.x platform (the `pioarduino` fork)** is what the code expects; the platform is not
  pinned to it in the repo.
- **`eve/platformio.ini`**: `build_src_filter = +<src/>` is relative to the default
  `src_dir` (`eve/src`), so PlatformIO looks for `eve/src/src/` and reports "Nothing to
  build" even though sources exist in `eve/src/`.
- **`audio_esp/`**: sources (`*.ino`/`*.cpp`) live at the module root but the ini does not set
  `src_dir = .`, so the default empty `src/` yields "Nothing to build".
- **`dock_station/platformio.ini`**: `lib_deps` entry `adafruit/Adafruit VL6180X` fails
  registry resolution (`UnknownPackageError`).

The PlatformIO environment itself is healthy: the platform/toolchains install and hundreds
of source files compile before these repo-level issues stop the build.

### Reference
- Firmware envs and flash steps: `PLATFORMIO_QUICK_START.md`, each module's `README.md`.
- Protocol/architecture: `ARCHITECTURE.md`. Contributor conventions: `CONTRIBUTING.md`.
- `webui/build-embed.ps1` (PowerShell) regenerates the embedded
  `main_wall_E_base/main/web_page_lros.h`; it is a build helper, not a runnable app.
