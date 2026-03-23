/*******************************************************************************
 * dock_test.h
 * Serial command set for full dock testing (sensors, outputs, TFT, NeoPixel)
 ******************************************************************************/

#ifndef DOCK_TEST_H
#define DOCK_TEST_H

#include <stdbool.h>

/* Command codes returned by dockTestCheckSerial() — 0 = no command.
 * Prefix DOCK_TEST_CMD_ to avoid clash with dock_protocol.h DockCommand enum (DOCK_CMD_*). */
#define DOCK_TEST_CMD_TEST     1
#define DOCK_TEST_CMD_CALL     2
#define DOCK_TEST_CMD_RESET    3
#define DOCK_TEST_CMD_HELP     4
#define DOCK_TEST_CMD_STATUS   5
#define DOCK_TEST_CMD_SENSORS  6
#define DOCK_TEST_CMD_GATEON   7
#define DOCK_TEST_CMD_GATEOFF  8
#define DOCK_TEST_CMD_ARROWL   9
#define DOCK_TEST_CMD_ARROR   10
#define DOCK_TEST_CMD_LEDON   11
#define DOCK_TEST_CMD_LEDOFF  12

/* Run one full test sequence (sensors → outputs → NeoPixel → TFT). Blocks ~5–8 s. */
void dockTestRun(void);

/* Run call-button test. Type 'q' to quit early. */
void dockTestCallButton(void);

/* Non-blocking: read Serial line, return command code (0 = none). */
int dockTestCheckSerial(void);

/* Execute a command (HELP, STATUS, SENSORS, GATEON, etc.). Call after dockTestCheckSerial() returns non-zero. */
void dockTestExecuteCommand(int cmd);

#endif /* DOCK_TEST_H */
