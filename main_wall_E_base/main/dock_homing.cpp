// ============================================================
//  WALL-E Dock Homing — RSSI-based homing toward beacon
// ============================================================

#include "dock_homing.h"
#include "dock_sensors.h"
#include "motor_control.h"
#include <Arduino.h>

#define HOMING_SPEED        120
#define HOMING_SEEK_SPEED   80
#define RSSI_THRESHOLD      -75    /* Must hear beacon better than this */
#define BEACON_TIMEOUT_MS   3000   /* Abort if no beacon this long */

/* Stray detection: auto-return when connection to dock fades */
#define STRAY_NO_BEACON_MS      5000   /* No beacon for 5s = strayed, return */
#define STRAY_RSSI_THRESHOLD    -85    /* RSSI weaker than this = far from dock */
#define STRAY_RSSI_DURATION_MS  3000   /* RSSI weak for 3s = trigger return */
#define SEEK_SAMPLE_MS      300    /* Sample RSSI every N ms during seek */
#define DRIVE_STRAIGHT_MS   800    /* Drive straight before re-seeking */
#define DOCK_ARRIVED_RSSI   -55    /* Very close; slow down */
#define SLOW_SPEED          60

enum HomingState {
  HOMING_IDLE,
  HOMING_SEEK_LEFT,   /* Spinning left to find best RSSI */
  HOMING_SEEK_RIGHT,
  HOMING_DRIVE,       /* Driving toward beacon */
  HOMING_ARRIVED,
  HOMING_ABORT
};

static bool       s_requested = false;
static HomingState s_state = HOMING_IDLE;
static int8_t     s_last_rssi = -100;
static uint32_t   s_last_beacon_ms = 0;
static int8_t     s_best_rssi = -100;
static int8_t     s_rssi_after_left = -100;  /* Best RSSI after left spin */
static int        s_best_direction = 0;  /* -1 left, 0 forward, 1 right */
static uint32_t   s_state_enter_ms = 0;
static int16_t    s_out_left = 0, s_out_right = 0;
static uint32_t   s_stray_weak_rssi_start_ms = 0;  /* When RSSI first went weak */

void dockHomingOnBeacon(int8_t rssi) {
  s_last_rssi = rssi;
  s_last_beacon_ms = millis();
  if (rssi < STRAY_RSSI_THRESHOLD) {
    if (s_stray_weak_rssi_start_ms == 0) s_stray_weak_rssi_start_ms = millis();
  } else {
    s_stray_weak_rssi_start_ms = 0;
  }
}

void dockHomingSetRequested(bool requested) {
  s_requested = requested;
  if (requested && s_state == HOMING_IDLE) {
    s_state = HOMING_SEEK_LEFT;
    s_state_enter_ms = millis();
    s_best_rssi = s_last_rssi;
  }
  if (!requested) {
    s_state = HOMING_IDLE;
  }
}

bool dockHomingIsRequested(void) {
  return s_requested;
}

bool dockHomingIsActive(void) {
  return s_state != HOMING_IDLE && s_state != HOMING_ARRIVED && s_state != HOMING_ABORT;
}

bool dockHomingGetMotorOutput(int16_t *left, int16_t *right) {
  if (!left || !right) return false;
  *left = s_out_left;
  *right = s_out_right;
  return s_state != HOMING_IDLE && s_state != HOMING_ABORT;
}

bool dockHomingUpdate(uint32_t now) {
  if (!s_requested && s_state == HOMING_IDLE) return false;
  if (s_state == HOMING_ARRIVED || s_state == HOMING_ABORT) {
    s_out_left = 0;
    s_out_right = 0;
    return s_state == HOMING_ARRIVED;
  }

  s_out_left = 0;
  s_out_right = 0;

  /* Abort: no beacon ever and trying >10s, or had beacon and lost it */
  if (s_state_enter_ms > 0) {
    if (s_last_beacon_ms == 0 && (now - s_state_enter_ms) > 10000) {
      s_state = HOMING_ABORT;
      s_requested = false;
      motorStop();
      return true;
    }
    if (s_last_beacon_ms > 0 && (now - s_last_beacon_ms) > BEACON_TIMEOUT_MS) {
      s_state = HOMING_ABORT;
      s_requested = false;
      motorStop();
      return true;
    }
  }

  /* Arrived: dock beam broken (robot sensors detect dock zone) */
  if (dockBeamPresent()) {
    s_state = HOMING_ARRIVED;
    s_requested = false;
    motorStop();
    return true;
  }

  /* SEEK: spin left then right, pick best RSSI direction */
  if (s_state == HOMING_SEEK_LEFT || s_state == HOMING_SEEK_RIGHT) {
    uint32_t elapsed = now - s_state_enter_ms;
    if (s_last_rssi > s_best_rssi) s_best_rssi = s_last_rssi;
    if (elapsed >= 4 * SEEK_SAMPLE_MS) {  /* ~1.2s per direction */
      if (s_state == HOMING_SEEK_LEFT) {
        s_rssi_after_left = s_best_rssi;
        s_state = HOMING_SEEK_RIGHT;
        s_state_enter_ms = now;
        s_best_rssi = -100;
      } else {
        /* Compare left vs right spin; pick direction with better RSSI */
        if (s_best_rssi > s_rssi_after_left + 2) s_best_direction = 1;
        else if (s_rssi_after_left > s_best_rssi + 2) s_best_direction = -1;
        else s_best_direction = 0;  /* Similar: drive straight */
        s_state = HOMING_DRIVE;
        s_state_enter_ms = now;
      }
    }
    s_out_left  = (s_state == HOMING_SEEK_LEFT) ? -HOMING_SEEK_SPEED : HOMING_SEEK_SPEED;
    s_out_right = (s_state == HOMING_SEEK_LEFT) ? HOMING_SEEK_SPEED : -HOMING_SEEK_SPEED;
    return true;
  }

  /* DRIVE: go toward beacon */
  if (s_state == HOMING_DRIVE) {
    uint32_t elapsed = now - s_state_enter_ms;
    int speed = (s_last_rssi >= DOCK_ARRIVED_RSSI) ? SLOW_SPEED : HOMING_SPEED;
    if (s_last_rssi < RSSI_THRESHOLD && elapsed > 500) {
      /* Lost signal; re-seek */
      s_state = HOMING_SEEK_LEFT;
      s_state_enter_ms = now;
      s_best_rssi = -100;
      return true;
    }
    /* best_direction: -1 = dock to our left, turn left. 1 = dock to right, turn right. 0 = straight */
    if (s_best_direction == -1) {
      s_out_left  = -speed;
      s_out_right = speed;
    } else if (s_best_direction == 1) {
      s_out_left  = speed;
      s_out_right = -speed;
    } else {
      s_out_left  = speed;
      s_out_right = speed;
    }
    return true;
  }

  return false;
}

void dockHomingCheckAutoReturn(uint32_t now) {
  if (dockHomingIsRequested() || dockHomingIsActive()) return;
  /* Never heard dock (boot) – don’t trigger */
  if (s_last_beacon_ms == 0) return;
  /* No beacon for a while → strayed, trigger return */
  if ((now - s_last_beacon_ms) > STRAY_NO_BEACON_MS) {
    dockHomingSetRequested(true);
    Serial.println("[Dock] Auto-return: no beacon (strayed too far)");
    return;
  }
  /* Beacon weak for sustained period → getting far, trigger return */
  if (s_stray_weak_rssi_start_ms > 0 && (now - s_stray_weak_rssi_start_ms) > STRAY_RSSI_DURATION_MS) {
    dockHomingSetRequested(true);
    Serial.println("[Dock] Auto-return: beacon weak (returning to range)");
    s_stray_weak_rssi_start_ms = 0;
  }
}
