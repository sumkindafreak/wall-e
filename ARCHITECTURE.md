# WALL-E — High-Level Architecture

This document describes the **multi-controller philosophy**, **node health**, **emotion pose** library, **docking guidance**, and **OTA** flow. For folder layout proposals see [FOLDER_STRUCTURE.md](FOLDER_STRUCTURE.md).

---

## Multi-controller design philosophy

### Separation of concerns

| Concern | Typical owner |
|---------|----------------|
| Operator intent (speed, mode, estop) | Master controller |
| Physics (motors, servos, safety timers) | Base |
| Dock-specific policy (charge, arrows, IR) | Dock |
| Specialized sensing (audio direction, vision) | Satellite nodes |

The base **does not** run the touchscreen UI; the master **does not** directly drive H-bridge pins. Commands cross a **binary protocol** (ESP-NOW) with explicit structs.

### Why ESP-NOW + Wi-Fi

- **ESP-NOW** — Low framing overhead for control loops and sensor frames; works alongside Wi-Fi when channel coordination is respected.
- **Wi-Fi** — AP for provisioning, HTTP for LROS, OTA, and optional home-LAN connectivity.

---

## Node health protocol

**Magic:** `WNHT` (`0x574E4854u`), **version** `1`.

Defined in [main_wall_E_base/main/node_health_protocol.h](main_wall_E_base/main/node_health_protocol.h) (copies exist under other modules).

### Node IDs (`walle_node_id_t`)

| ID | Constant | Role |
|----|----------|------|
| 0 | `WALLE_NODE_BASE` | Base brain |
| 1 | `WALLE_NODE_MASTER` | Master controller |
| 2 | `WALLE_NODE_AUDIO` | Audio |
| 3 | `WALLE_NODE_DOCK` | Dock |
| 4 | `WALLE_NODE_VISION` | Vision |

Packets include **battery %**, **temperature**, **uptime**, **flags** (docked, charging, fault), and **last_error**. The base **registry** merges last-seen timestamps for UI and logic.

**Operational rule:** Keep copies of the header **byte-identical** across nodes until a single shared include is adopted.

---

## Emotion pose library

Location: [`lib/walle_emotion_pose/`](lib/README.md) and thin wrappers in `main_wall_e_base` / `wall_e_master_controller`.

**Purpose:** Decouple **high-level emotional state** and **pose triggers** from specific servo PWM wiring. The library exposes a small state machine and hooks; the base applies outputs to the PCA9685 or similar via existing servo code.

**Integration:** Bridge modules may map ESP-NOW events (audio, vision) into pose triggers without duplicating geometry math in every node.

---

## Docking guidance logic

### Layers

1. **ESP-NOW homing** — Dock `DockBeaconPacket_t` at ~10 Hz; base uses RSSI and `dock_id` for search.
2. **IR alignment** — Robot **IR transmitters** (~38 kHz); dock **TSOP receivers** on left/right; firmware classifies **LEFT / RIGHT / CENTER / LOST**, drives **arrow MOSFETs**, and sets **`ir_align_hint`** on the beacon when Wi-Fi/ESP-NOW is enabled on the dock.
3. **Approach stage** — WALL-E reports `APPROACH_*` stages via ESP-NOW; dock falls back to IR sensors after timeout.
4. **Presence** — VL6180 ToF (I2C), optional IR break-beam, obstacle sensors — see `dock_config.h` and `dock_sensors.cpp`.
5. **Charge** — ACS712 current thresholds and MOSFET gate control in the dock state machine.

**Do not mix** break-beam “presence” with alignment IR classification in documentation; they are separate inputs.

---

## OTA update flow

1. **Build** firmware with PlatformIO (`.pio/build/.../firmware.bin`).
2. **Base** — Often updated via **web** at `http://192.168.4.1/update` on the WALL-E AP, or **ArduinoOTA** on port 3232.
3. **Controller / dock** — `upload_protocol = espota` with target IP/host; see [OTA_README.md](OTA_README.md).
4. **Scripts** — `ota_build_all.ps1` / `ota_build_all.sh` for batch builds (optional upload).

mDNS hostnames (`wall-e-base.local`, etc.) may work depending on LAN multicast.

---

## Control protocol (master ↔ base)

**ControlPacket** (packed): left/right speed, drive mode, behaviour mode, action, system flags, servo targets array.

**TelemetryPacket** (packed): battery, current, temperature, mood, autonomy fields, etc.

Exact layouts: [wall_e_master_controller/protocol.h](wall_e_master_controller/protocol.h).

---

## Vision path

**VisionPacket_t** — `VISION_MAGIC`, motion flag, target position, bbox, class, frame ID. Sent ESP-NOW to base; base **vision_behaviour** consumes packets for servo tracking.

---

## Dock beacon and commands

- **DockBeaconPacket_t** — `DOCK_BEACON_MAGIC`, state, beam, mouth, charge, callout, current, **`ir_align_hint`**.
- **Commands** — e.g. `DOCK_CMD_REQUEST_CHARGE`, `DOCK_CMD_APPROACH_STAGE` — see [dock_station/dock_protocol.h](dock_station/dock_protocol.h).

---

## Future consolidation

- Single source for `node_health_protocol.h` and `dock_protocol.h`
- Versioned protocol structs with backward-compatible parsing
- Centralized channel/MAC configuration
