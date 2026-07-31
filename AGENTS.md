# AGENTS.md

## Cursor Cloud specific instructions

Multi-node **ESP32/ESP32-S3 PlatformIO** monorepo + static **`webui/` (LROS)**. See `README.md` and `ARCHITECTURE.md`.

### Toolchain
- **PlatformIO Core**: `pip install --user platformio` → `~/.local/bin/pio` (on PATH via `~/.profile` / `~/.bashrc`).
- **pioarduino (Arduino ESP32 Core 3.x)**: All seven `platformio.ini` modules include `firmware_common/platformio_pioarduino.ini` (pinned **55.03.39** / Arduino **3.3.9**). First build downloads the custom platform into `~/.platformio`.
- **python3-venv**: pioarduino needs a working `python3 -m venv` for `~/.platformio/penv`. On Ubuntu: `sudo apt-get install -y python3.12-venv` if builds fail with penv errors.
- **`webui/`**: static only — `python3 -m http.server 8000` in `webui/`, open `index-standalone.html` for offline demo.

### Build all firmware (compile-only on Linux)
```bash
export PATH="$HOME/.local/bin:$PATH"
./scripts/build_all_firmware.sh
./scripts/collect_firmware_artifacts.sh   # -> dist/firmware/*.bin
```
Or per module: `cd <module> && pio run` (uses each folder's `default_envs`).

### CI
GitHub Actions workflow **`.github/workflows/firmware-build.yml`**: builds all seven targets, caches `~/.platformio`, uploads `dist/firmware/*.bin` artifacts (`wall_e_base.bin`, `eve.bin`, …).

### Firmware version
- Manual semver: `firmware_common/include/showduino_version.h` (`SHOWDUINO_VERSION_*`).
- Generated per build: `showduino_version_build.h` via `scripts/gen_showduino_version.sh` (`SHOWDUINO_BUILD`, `SHOWDUINO_GIT_HASH`; CI sets build from `GITHUB_RUN_NUMBER`).
- Boot log: `SHOWDUINO_LOG_BOOT_VERSION("module_tag")` in each PlatformIO sketch.

Verified targets (Core 3.x): **main_wall_E_base**, **eve**, **audio_esp**, **dock_station**, **wall_e_master_controller**, **vision_node**, **ghostbusters_slime_blower**.

### Shared PlatformIO helpers
- `firmware_common/platformio_pioarduino.ini` — platform pin + `extra_scripts`.
- `firmware_common/platformio_framework_includes.py` — framework `WiFi`/`Network`/`NetworkClientSecure` include paths; skips LVGL `.S` asm under `.pio/libdeps`.

### Notes
- Flashing/running firmware requires physical boards; the VM only cross-compiles.
- `webui/build-embed.ps1` is PowerShell-only (regenerates embedded LROS header in base firmware).
- Protocol header check: `./scripts/verify_protocol_headers.sh` (also run by `ota_build_all.sh`).
