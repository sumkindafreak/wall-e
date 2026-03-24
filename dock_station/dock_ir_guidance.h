/*******************************************************************************
 * dock_ir_guidance.h
 * Dual-side IR beacon alignment: dock reads TSOP/VS1838B receivers (LOW = IR).
 * Modular API: readIRSensors, getDockAlignment, processDockGuidance.
 ******************************************************************************/

#ifndef DOCK_IR_GUIDANCE_H
#define DOCK_IR_GUIDANCE_H

#include "dock_config.h"
#include <stdint.h>
#include <stdbool.h>

/* Call once after pins are configured as INPUT (dockAlignmentBegin does that). */
void dockIrGuidanceBegin(void);

/* Raw instant read (no debounce). TSOP: LOW on pin = true. */
void readIRSensors(bool *leftDetected, bool *rightDetected);

/* Re-print guidance for current debounced state (uses last serialVerbose from Update). */
void processDockGuidance(void);

/* Poll every loop. robotDocked = beam/VL6180 “in slot”; serialVerbose = fewer prints during callout. */
void dockIrGuidanceUpdate(uint32_t now_ms, bool robotDocked, bool serialVerbose);

/* Latest debounced classification (matches Serial keywords). */
const char *getDockAlignment(void);

/* Byte for DockBeaconPacket_t.ir_align_hint — see dock_protocol.h */
uint8_t dockIrGuidanceGetBeaconHint(void);

bool dockIrGuidanceGuidancePaused(void);
bool dockIrGuidanceAnyReceiverActive(void);

/* Debounced class: 0=LOST, 1=LEFT, 2=RIGHT, 3=CENTER (matches IrStable_t order). */
uint8_t dockIrGuidanceStableId(void);

bool dockIrGuidanceDebugLeftOn(void);
bool dockIrGuidanceDebugRightOn(void);

#endif /* DOCK_IR_GUIDANCE_H */
