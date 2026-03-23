/*******************************************************************************
 * dock_test.cpp
 * Full serial command set for dock testing (sensors, outputs, TFT, NeoPixel)
 ******************************************************************************/

#include "dock_test.h"
#include "dock_config.h"
#include "dock_sensors.h"
#include "dock_state.h"
#include "dock_hw.h"
#include "dock_display.h"
#include "dock_neopixel.h"
#include "dock_protocol.h"
#include "dock_callout.h"
#include <Arduino.h>
#include <string.h>

#define TEST_STEP_MS    800
#define CALL_TEST_DURATION_MS  15000
#define CALL_TEST_INTERVAL_MS  300
#define PULSE_MS        1000   /* Output test pulse duration */
#define ARR_ON   (MOSFET_ARROW_ACTIVE_LOW ? LOW : HIGH)
#define ARR_OFF  (MOSFET_ARROW_ACTIVE_LOW ? HIGH : LOW)
#define INT_ON   (MOSFET_INTERNAL_ACTIVE_LOW ? LOW : HIGH)
#define INT_OFF  (MOSFET_INTERNAL_ACTIVE_LOW ? HIGH : LOW)

#define SERIAL_BUF_SIZE  20
static char s_serial_buf[SERIAL_BUF_SIZE];
static size_t s_serial_len = 0;

static bool cmdEquals(const char* cmd) {
  size_t n = strlen(cmd);
  if (s_serial_len != n) return false;
  for (size_t i = 0; i < n; i++) {
    if ((char)s_serial_buf[i] != cmd[i]) return false;
  }
  return true;
}

void dockTestRun(void) {
  Serial.println(F("\n=== DOCK TEST ==="));

  /* 1. Sensors */
  dockSensorsUpdate();
  Serial.print(F("  Beam: "));
  Serial.println(dockBeamPresent() ? F("YES") : F("no"));
  Serial.print(F("  Mouth/obstacles: "));
  Serial.println(dockMouthBlocked() ? F("BLOCKED") : F("clear"));
  Serial.print(F("  Current: "));
  Serial.print(dockCurrentAmps(), 3);
  Serial.println(F(" A"));
  Serial.print(F("  Call switch: "));
  Serial.println((dockDigitalReadSafe(PIN_CALL_SWITCH, "call switch", HIGH) == LOW) ? F("ON (calling)") : F("off"));

  /* 2. TFT: show current state */
  dockDisplayUpdate();
  delay(200);

  /* 3. Outputs: charge gate (brief pulse only if available) */
  Serial.println(F("  Outputs: cycling..."));
  if (dockChargeGateAvailable()) {
    dockChargeGateWrite(true);
    Serial.println(F("    Charge gate ON"));
    delay(TEST_STEP_MS);
    dockChargeGateWrite(false);
    Serial.println(F("    Charge gate OFF"));
    delay(200);
  } else {
    Serial.println(F("    Charge gate (skip - not available)"));
  }

  /* 4. Arrow left */
  dockWriteOutputPin(PIN_ARROW_LEFT, ARR_ON, "left arrow");
  Serial.println(F("    Arrow L ON"));
  delay(TEST_STEP_MS);
  dockWriteOutputPin(PIN_ARROW_LEFT, ARR_OFF, "left arrow");
  delay(200);

  /* 5. Arrow right */
  dockWriteOutputPin(PIN_ARROW_RIGHT, ARR_ON, "right arrow");
  Serial.println(F("    Arrow R ON"));
  delay(TEST_STEP_MS);
  dockWriteOutputPin(PIN_ARROW_RIGHT, ARR_OFF, "right arrow");
  delay(200);

  /* 6. Internal LED */
  dockWriteOutputPin(PIN_INTERNAL_LED, INT_ON, "internal LED");
  Serial.println(F("    Internal LED ON"));
  delay(TEST_STEP_MS);
  dockWriteOutputPin(PIN_INTERNAL_LED, INT_OFF, "internal LED");
  delay(200);

  /* 7. NeoPixel: red → green → off */
  dockNeoPixelUpdateEx(NP_STATE_FAULT, false, FAULT_NONE, false, false, 0.0f);
  Serial.println(F("    NeoPixel RED"));
  delay(TEST_STEP_MS);
  dockNeoPixelUpdateEx(NP_STATE_CHARGED, false, FAULT_NONE, false, false, 0.0f);
  Serial.println(F("    NeoPixel GREEN"));
  delay(TEST_STEP_MS);
  dockNeoPixelUpdateEx(NP_STATE_NOT_DOCKED, false, FAULT_NONE, false, false, 0.0f);
  delay(200);

  /* 8. Final TFT update */
  dockSensorsUpdate();
  dockDisplayUpdate();

  Serial.println(F("=== TEST DONE ===\n"));
}

void dockTestCallButton(void) {
  Serial.println(F("\n=== CALL BUTTON TEST ==="));
  Serial.println(F("Press/release the call button. Arrows + internal LED will follow. Type 'q' to quit."));
  Serial.println(F("(Runs 15 s or until 'q')\n"));

  uint32_t start = millis();
  bool last_state = false;
  bool first = true;

  while ((millis() - start) < CALL_TEST_DURATION_MS) {
    /* Check for 'q' to quit */
    if (Serial.available()) {
      char c = (char)Serial.read();
      if (c == 'q' || c == 'Q') {
        Serial.println(F("  [quit]"));
        break;
      }
    }

    dockCalloutUpdate();  /* so arrows + LED respond to the button */
    bool pressed = (dockDigitalReadSafe(PIN_CALL_SWITCH, "call switch", HIGH) == LOW);

    if (pressed != last_state || first) {
      last_state = pressed;
      first = false;
      Serial.print(F("  Call: "));
      Serial.println(pressed ? F("PRESSED (calling)") : F("released"));
    }

    delay(CALL_TEST_INTERVAL_MS);
  }

  Serial.println(F("=== CALL TEST DONE ===\n"));
}

int dockTestCheckSerial(void) {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      s_serial_buf[s_serial_len] = '\0';
      int cmd = 0;
      if (cmdEquals("HELP"))     cmd = DOCK_TEST_CMD_HELP;
      else if (cmdEquals("STATUS"))   cmd = DOCK_TEST_CMD_STATUS;
      else if (cmdEquals("SENSORS"))  cmd = DOCK_TEST_CMD_SENSORS;
      else if (cmdEquals("TEST"))     cmd = DOCK_TEST_CMD_TEST;
      else if (cmdEquals("CALL"))     cmd = DOCK_TEST_CMD_CALL;
      else if (cmdEquals("RESET"))    cmd = DOCK_TEST_CMD_RESET;
      else if (cmdEquals("GATEON"))   cmd = DOCK_TEST_CMD_GATEON;
      else if (cmdEquals("GATEOFF"))  cmd = DOCK_TEST_CMD_GATEOFF;
      else if (cmdEquals("ARROWL"))   cmd = DOCK_TEST_CMD_ARROWL;
      else if (cmdEquals("ARROR"))    cmd = DOCK_TEST_CMD_ARROR;
      else if (cmdEquals("LEDON"))    cmd = DOCK_TEST_CMD_LEDON;
      else if (cmdEquals("LEDOFF"))   cmd = DOCK_TEST_CMD_LEDOFF;
      s_serial_len = 0;
      if (cmd) return cmd;
      continue;
    }
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);  /* uppercase */
    if (s_serial_len < SERIAL_BUF_SIZE - 1) {
      s_serial_buf[s_serial_len++] = (char)c;
    } else {
      s_serial_len = 0;
    }
  }
  return 0;
}

static void printHelp(void) {
  Serial.println(F("\n=== DOCK SERIAL COMMANDS (115200 baud) ==="));
  Serial.println(F("  HELP     - This list"));
  Serial.println(F("  STATUS   - State, beam, mouth, current, gate"));
  Serial.println(F("  SENSORS  - Beam, obstacles FL/FR/BL/BR, current raw/zero/amps, call switch"));
  Serial.println(F("  TEST     - Full cycle: sensors, TFT, gate pulse, arrows, LED, NeoPixel"));
  Serial.println(F("  CALL     - Call-button test (15s); 'q' to quit"));
  Serial.println(F("  RESET    - Clear FAULT state"));
  Serial.println(F("  GATEON   - Charge gate ON for 1s (then off)"));
  Serial.println(F("  GATEOFF  - Charge gate OFF"));
  Serial.println(F("  ARROWL   - Arrow left ON for 1s"));
  Serial.println(F("  ARROR    - Arrow right ON for 1s"));
  Serial.println(F("  LEDON    - Internal LED ON for 1s"));
  Serial.println(F("  LEDOFF   - Internal LED OFF"));
  Serial.println(F("==========================================\n"));
}

static void printStatus(void) {
  dockSensorsUpdate();
  Serial.println(F("\n--- STATUS ---"));
  DockState s = dockStateGet();
  Serial.print(F("  State: "));
  switch (s) {
    case STATE_BOOT:        Serial.println(F("BOOT")); break;
    case STATE_NOT_DOCKED:  Serial.println(F("NOT_DOCKED")); break;
    case STATE_DOCKED_IDLE: Serial.println(F("DOCKED_IDLE")); break;
    case STATE_CHARGING:    Serial.println(F("CHARGING")); break;
    case STATE_CHARGED:     Serial.println(F("CHARGED")); break;
    case STATE_FAULT:      Serial.println(dockStateGetFaultCode() == FAULT_OVERCURRENT ? F("FAULT_OVC") : F("FAULT_OFF")); break;
    default: Serial.println(F("?")); break;
  }
  Serial.print(F("  Beam: "));      Serial.println(dockBeamPresent() ? F("YES") : F("no"));
  Serial.print(F("  Mouth: "));     Serial.println(dockMouthBlocked() ? F("BLOCKED") : F("clear"));
  Serial.print(F("  Current: "));   Serial.print(dockCurrentAmps(), 3); Serial.println(F(" A"));
  Serial.print(F("  Gate: "));     Serial.println(dockChargeEnabled() ? F("ON") : F("off"));
  Serial.print(F("  Idle: "));      Serial.println(dockIsIdleMode() ? F("yes") : F("no"));
  Serial.println(F("---------------\n"));
}

static void printSensors(void) {
  dockSensorsUpdate();
  Serial.println(F("\n--- SENSORS ---"));
  Serial.print(F("  Beam: "));     Serial.println(dockBeamPresent() ? F("YES") : F("no"));
  Serial.print(F("  Obst FL: "));  Serial.println(dockObstacleBlocked(0) ? F("BLOCKED") : F("clear"));
  Serial.print(F("  Obst FR: "));  Serial.println(dockObstacleBlocked(1) ? F("BLOCKED") : F("clear"));
  Serial.print(F("  Obst BL: "));  Serial.println(dockObstacleBlocked(2) ? F("BLOCKED") : F("clear"));
  Serial.print(F("  Obst BR: "));  Serial.println(dockObstacleBlocked(3) ? F("BLOCKED") : F("clear"));
  Serial.print(F("  Current: "));  Serial.print(dockCurrentAmps(), 3); Serial.println(F(" A"));
  Serial.print(F("  Raw: "));      Serial.print(dockCurrentRaw()); Serial.print(F("  Zero: ")); Serial.println(dockCurrentZero());
  Serial.print(F("  Call: "));     Serial.println((dockDigitalReadSafe(PIN_CALL_SWITCH, "call", HIGH) == LOW) ? F("PRESSED") : F("released"));
  Serial.println(F("----------------\n"));
}

void dockTestExecuteCommand(int cmd) {
  switch (cmd) {
    case DOCK_TEST_CMD_HELP:
      printHelp();
      break;
    case DOCK_TEST_CMD_STATUS:
      printStatus();
      break;
    case DOCK_TEST_CMD_SENSORS:
      printSensors();
      break;
    case DOCK_TEST_CMD_TEST:
      dockTestRun();
      break;
    case DOCK_TEST_CMD_CALL:
      dockTestCallButton();
      break;
    case DOCK_TEST_CMD_RESET:
      dockStateResetFault();
      Serial.println(F("[DOCK] Fault reset"));
      break;
    case DOCK_TEST_CMD_GATEON:
      if (dockChargeGateAvailable()) {
        dockChargeGateWrite(true);
        Serial.println(F("[DOCK] Gate ON 1s"));
        delay(PULSE_MS);
        dockChargeGateWrite(false);
      } else Serial.println(F("[DOCK] Gate not available"));
      break;
    case DOCK_TEST_CMD_GATEOFF:
      dockChargeGateWrite(false);
      Serial.println(F("[DOCK] Gate OFF"));
      break;
    case DOCK_TEST_CMD_ARROWL:
      dockWriteOutputPin(PIN_ARROW_LEFT, ARR_ON, "arrow L");
      Serial.println(F("[DOCK] Arrow L ON 1s"));
      delay(PULSE_MS);
      dockWriteOutputPin(PIN_ARROW_LEFT, ARR_OFF, "arrow L");
      break;
    case DOCK_TEST_CMD_ARROR:
      dockWriteOutputPin(PIN_ARROW_RIGHT, ARR_ON, "arrow R");
      Serial.println(F("[DOCK] Arrow R ON 1s"));
      delay(PULSE_MS);
      dockWriteOutputPin(PIN_ARROW_RIGHT, ARR_OFF, "arrow R");
      break;
    case DOCK_TEST_CMD_LEDON:
      dockWriteOutputPin(PIN_INTERNAL_LED, INT_ON, "LED");
      Serial.println(F("[DOCK] LED ON 1s"));
      delay(PULSE_MS);
      dockWriteOutputPin(PIN_INTERNAL_LED, INT_OFF, "LED");
      break;
    case DOCK_TEST_CMD_LEDOFF:
      dockWriteOutputPin(PIN_INTERNAL_LED, INT_OFF, "LED");
      Serial.println(F("[DOCK] LED OFF"));
      break;
    default:
      break;
  }
}
