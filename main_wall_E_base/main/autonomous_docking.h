/*******************************************************************************
 * autonomous_docking.h
 * WALL-E Autonomous Docking — State Machine
 *
 * Full navigation and docking logic for life-size WALL-E:
 * 1. Detect low battery
 * 2. Search for dock (ESP-NOW beacon RSSI)
 * 3. WALL-E IR TX on during search/align/approach; dock’s two RX → ir_align_hint on beacon
 * 4. Approach using ToF distance (speed ramps down as distance decreases)
 * 5. Enter dock when ToF reads close (no onboard IR break-beam)
 * 6. Send REQUEST_CHARGE via ESP-NOW
 * 7. Stop and enter CHARGING
 *
 * Integrates with: motor_control, vl53l1x_tof, dock_sensors, battery_monitor,
 *                  dock_controller, dock_ir_transmitters
 ******************************************************************************/

#ifndef AUTONOMOUS_DOCKING_H
#define AUTONOMOUS_DOCKING_H

#include <stdint.h>
#include <stdbool.h>

/*-----------------------------------------------------------------------------
 * Docking states
 *----------------------------------------------------------------------------*/
typedef enum {
  DOCK_STATE_IDLE,       /* Normal operation until battery low */
  DOCK_STATE_SEARCH,     /* Rotate, listen for beacon / IR */
  DOCK_STATE_ALIGN,      /* Steer to balance IR left/right */
  DOCK_STATE_APPROACH,   /* Drive toward dock, ToF-controlled speed */
  DOCK_STATE_DOCKED,     /* IR beam confirms; send charge request */
  DOCK_STATE_CHARGING    /* Stop, remain stationary */
} DockState;

/*-----------------------------------------------------------------------------
 * Configuration (tune for your hardware)
 *----------------------------------------------------------------------------*/
#define DOCK_BATTERY_LOW_PCT       20   /* Start search when below this % */
#define DOCK_BATTERY_OK_PCT        50   /* Can cancel auto-dock above this */
#define DOCK_IR_ALIGN_THRESHOLD    20   /* |left-right| < this = aligned */
#define DOCK_TOF_NORMAL_MM         200  /* Distance > this: normal speed */
#define DOCK_TOF_SLOW_MM           120  /* 120–200: slow speed */
#define DOCK_TOF_CRAWL_MM          60   /* 60–120: crawl; <60: stop → DOCKED */
#define DOCK_BEACON_TIMEOUT_MS     8000 /* Abort search if no beacon */
#define DOCK_ALIGN_TIMEOUT_MS      15000
#define DOCK_APPROACH_TIMEOUT_MS   30000

/*-----------------------------------------------------------------------------
 * API
 *----------------------------------------------------------------------------*/

/* Call once after WiFi/ESP-NOW, ToF, dock sensors, battery init */
void autonomousDockingInit(void);

/* Feed ESP-NOW beacon RSSI and dock_id (call from recv callback) */
void autonomousDockingOnBeacon(int8_t rssi);
void autonomousDockingSetLastDockId(uint32_t dock_id);
/* Latest DockBeaconPacket_t.ir_align_hint (DOCK_IR_ALIGN_* in dock_protocol.h) */
void autonomousDockingOnIrAlign(uint8_t ir_align_hint);

/* Call every loop. Returns true if docking is active and producing motor output */
bool autonomousDockingUpdate(uint32_t now);

/* Get motor output when active. left/right -255..255 */
bool autonomousDockingGetMotorOutput(int16_t* left, int16_t* right);

/* True if in any docking state except IDLE */
bool autonomousDockingIsActive(void);

/* Current state (for UI, telemetry, debug) */
DockState autonomousDockingGetState(void);

/* Manually request dock (e.g. from controller) */
void autonomousDockingSetRequested(bool requested);

const char* autonomousDockingGetStateName(void);

#endif /* AUTONOMOUS_DOCKING_H */
