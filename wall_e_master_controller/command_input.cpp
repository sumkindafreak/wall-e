// ============================================================
//  Serial + optional physical button macros → command queue
// ============================================================

#include "command_input.h"
#include "command_queue.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static char     s_line[96];
static uint8_t  s_lineLen = 0;

static int parseIntToken(const char* s, int defVal) {
  if (!s || !*s) return defVal;
  return atoi(s);
}

static void printHelp(void) {
  Serial.println(F(
    "\n--- CYD command queue (Serial, end line with Enter) ---\n"
    "  FWD [0-100]     pulse forward\n"
    "  BACK [0-100]    pulse backward\n"
    "  TL [0-100]      turn left (differential)\n"
    "  TR [0-100]      turn right\n"
    "  STOP            STOP_ALL (queue + base action)\n"
    "  LASER ON|OFF|1|0\n"
    "  HEAD [0-180]    head pan degrees (one-shot)\n"
    "  TILT [0-180]    head tilt degrees (one-shot)\n"
    "  HELP            this text\n"
  ));
}

static void handleLine(char* line) {
  while (*line == ' ' || *line == '\t') line++;
  if (*line == '\0' || *line == '\r') return;

  char* save = nullptr;
  char* tok = strtok_r(line, " \t\r\n", &save);
  if (!tok) return;

  char cmd[12] = {0};
  size_t i = 0;
  while (tok[i] && i < sizeof(cmd) - 1) {
    char c = tok[i];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    cmd[i] = c;
    i++;
  }

  char* arg1 = strtok_r(nullptr, " \t\r\n", &save);

  int n = arg1 ? parseIntToken(arg1, 70) : 70;
  if (n < 0) n = 0;
  if (n > 100) n = 100;

  if (!strcmp(cmd, "HELP") || !strcmp(cmd, "?")) {
    printHelp();
    return;
  }
  if (!strcmp(cmd, "FWD") || !strcmp(cmd, "MF")) {
    commandQueuePush(CmdType_MOVE_FORWARD, (int16_t)n, 0);
    Serial.printf("[Serial] FWD %d%%\n", n);
    return;
  }
  if (!strcmp(cmd, "BACK") || !strcmp(cmd, "REV")) {
    commandQueuePush(CmdType_MOVE_BACKWARD, (int16_t)n, 0);
    Serial.printf("[Serial] BACK %d%%\n", n);
    return;
  }
  if (!strcmp(cmd, "TL") || !strcmp(cmd, "TURNLEFT")) {
    commandQueuePush(CmdType_TURN_LEFT, (int16_t)n, 0);
    Serial.printf("[Serial] TL %d%%\n", n);
    return;
  }
  if (!strcmp(cmd, "TR") || !strcmp(cmd, "TURNRIGHT")) {
    commandQueuePush(CmdType_TURN_RIGHT, (int16_t)n, 0);
    Serial.printf("[Serial] TR %d%%\n", n);
    return;
  }
  if (!strcmp(cmd, "STOP") || !strcmp(cmd, "HALT") || !strcmp(cmd, "ESTOP")) {
    commandQueuePush(CmdType_STOP_ALL, 0, 0);
    Serial.println(F("[Serial] STOP_ALL"));
    return;
  }
  if (!strcmp(cmd, "LASER")) {
    if (!arg1) return;
    char a[8] = {0};
    strncpy(a, arg1, sizeof(a) - 1);
    for (size_t k = 0; a[k]; k++) {
      if (a[k] >= 'a' && a[k] <= 'z') a[k] = (char)(a[k] - 'a' + 'A');
    }
    if (!strcmp(a, "ON") || !strcmp(a, "1")) {
      commandQueuePush(CmdType_LASER_ON, 0, 0);
      Serial.println(F("[Serial] LASER_ON"));
    } else if (!strcmp(a, "OFF") || !strcmp(a, "0")) {
      commandQueuePush(CmdType_LASER_OFF, 0, 0);
      Serial.println(F("[Serial] LASER_OFF"));
    }
    return;
  }
  if (!strcmp(cmd, "HEAD")) {
    int d = arg1 ? parseIntToken(arg1, 90) : 90;
    if (d < 0) d = 0;
    if (d > 180) d = 180;
    commandQueuePush(CmdType_HEAD_ROTATE, (int16_t)d, 0);
    Serial.printf("[Serial] HEAD %d\n", d);
    return;
  }
  if (!strcmp(cmd, "TILT")) {
    int d = arg1 ? parseIntToken(arg1, 90) : 90;
    if (d < 0) d = 0;
    if (d > 180) d = 180;
    commandQueuePush(CmdType_HEAD_TILT, (int16_t)d, 0);
    Serial.printf("[Serial] TILT %d\n", d);
    return;
  }

  Serial.printf("[Serial] Unknown: %s (HELP for list)\n", cmd);
}

void commandInputInit(void) {
  s_lineLen = 0;
  s_line[0] = '\0';
  printHelp();
}

void commandInputPollSerial(void) {
  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) break;
    if (c == '\r') continue;
    if (c == '\n') {
      s_line[s_lineLen] = '\0';
      handleLine(s_line);
      s_lineLen = 0;
      continue;
    }
    if (s_lineLen < sizeof(s_line) - 1) {
      s_line[s_lineLen++] = (char)c;
    } else {
      s_lineLen = 0;
      Serial.println(F("[Serial] Line too long — reset"));
    }
  }
}

#if USE_CMD_BUTTON_MACROS
void commandInputPollButtonMacros(const ButtonState& btn) {
  if (btn.pressed[BTN_EXTRA3] && btn.pressed[BTN_EXTRA4]) {
    commandQueuePush(CmdType_STOP_ALL, 0, 0);
    Serial.println(F("[Btn] STOP_ALL (EXTRA3+EXTRA4)"));
    return;
  }
  if (btn.pressed[BTN_EXTRA1]) {
    commandQueuePush(CmdType_MOVE_FORWARD, 70, 0);
    Serial.println(F("[Btn] FWD 70%"));
  }
  if (btn.pressed[BTN_EXTRA2]) {
    commandQueuePush(CmdType_MOVE_BACKWARD, 70, 0);
    Serial.println(F("[Btn] BACK 70%"));
  }
  if (btn.pressed[BTN_EXTRA3]) {
    commandQueuePush(CmdType_TURN_LEFT, 60, 0);
    Serial.println(F("[Btn] TL 60%"));
  }
  if (btn.pressed[BTN_EXTRA4]) {
    commandQueuePush(CmdType_TURN_RIGHT, 60, 0);
    Serial.println(F("[Btn] TR 60%"));
  }
}
#endif
