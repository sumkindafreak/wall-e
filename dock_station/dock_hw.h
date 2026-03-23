/*******************************************************************************
 * dock_hw.h
 * Hardware safety helpers for board-specific GPIO capabilities
 ******************************************************************************/

#ifndef DOCK_HW_H
#define DOCK_HW_H

#include <Arduino.h>

bool dockConfigureInputPin(uint8_t pin, uint8_t mode, const char *label);
bool dockConfigureOutputPin(uint8_t pin, uint8_t initial_level, const char *label);
void dockWriteOutputPin(uint8_t pin, uint8_t level, const char *label);
int dockDigitalReadSafe(uint8_t pin, const char *label, int fallback = LOW);
int dockAnalogReadSafe(uint8_t pin, const char *label, int fallback = 0);
bool dockChargeGateAvailable(void);
void dockChargeGateWrite(bool enabled);
bool dockCurrentSenseAvailable(void);
bool dockOledPinsAvailable(void);

#endif /* DOCK_HW_H */