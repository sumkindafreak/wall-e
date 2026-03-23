# Autonomous Docking System

Production-quality docking behaviour for life-size WALL-E.

## State Machine

| State     | Description |
|-----------|-------------|
| **IDLE**  | Normal operation. Transitions to SEARCH when battery < 20% or on manual request. |
| **SEARCH**| Rotate slowly, listen for ESP-NOW beacon (RSSI) or IR beacon signals. |
| **ALIGN** | Steer using left/right IR receivers until `|left - right| < threshold`. |
| **APPROACH** | Drive forward with ToF-based speed: >200mm normal, 120–200 slow, 60–120 crawl, <60 stop → DOCKED. |
| **DOCKED** | IR beam confirms arrival; send REQUEST_CHARGE via ESP-NOW. |
| **CHARGING** | Motors off, stationary. |

## Sensors

- **VL53L1X ToF** (I2C) — approach distance
- **IR Beacon Left/Right** (GPIO 21, 38) — alignment (edit `ir_beacon_receivers.h` for your pins)
- **IR Break Beam** (`dock_sensors`) — dock arrival
- **ESP-NOW** — beacon RSSI and dock_id

## Configuration

`main/dock_config.h` — set `USE_AUTONOMOUS_DOCKING`:
- `1` — new autonomous docking FSM (default)
- `0` — legacy dock_homing (RSSI-based)

`main/autonomous_docking.h` — thresholds:
- `DOCK_BATTERY_LOW_PCT` 20
- `DOCK_TOF_NORMAL_MM` 200, `DOCK_TOF_SLOW_MM` 120, `DOCK_TOF_CRAWL_MM` 60
- `DOCK_IR_ALIGN_THRESHOLD` 20

## Files

| File | Purpose |
|------|---------|
| `autonomous_docking.cpp/h` | State machine, navigation logic |
| `ir_beacon_receivers.cpp/h` | Left/right IR reading for alignment |
| `dock_controller.cpp/h` | `dockControllerSendRequestCharge()` |
| `dock_config.h` | USE_AUTONOMOUS_DOCKING switch |

## Dock Station

When WALL-E sends `DOCK_CMD_REQUEST_CHARGE` and the IR beam is broken, the dock enables the charging MOSFET. See `dock_station/dock_state.cpp`.
