/**
 * ir_dock_receivers.h — Left/right IR dock receiver states
 * States: NONE, LEFT, RIGHT, BOTH, UNSTABLE
 */
#ifndef AUDIO_ESP_IR_DOCK_RECEIVERS_H
#define AUDIO_ESP_IR_DOCK_RECEIVERS_H

#include "config.h"
#include "pins.h"
#include "debug_log.h"
#include <Arduino.h>

enum DockIrState {
  DOCK_IR_NONE,
  DOCK_IR_LEFT,
  DOCK_IR_RIGHT,
  DOCK_IR_BOTH,
  DOCK_IR_UNSTABLE
};

/* Initialize GPIO */
void irDockInit();

/* Call every loop. Updates state with debounce. */
void irDockTick();

/* Get current dock alignment state */
DockIrState irDockGetState();

/* For protocol */
inline const char* dockIrToString(DockIrState s) {
  switch (s) {
    case DOCK_IR_NONE:    return "NONE";
    case DOCK_IR_LEFT:    return "LEFT";
    case DOCK_IR_RIGHT:   return "RIGHT";
    case DOCK_IR_BOTH:    return "BOTH";
    case DOCK_IR_UNSTABLE: return "UNSTABLE";
    default:              return "NONE";
  }
}

#endif
