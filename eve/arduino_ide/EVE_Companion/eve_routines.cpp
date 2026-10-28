#include "eve_routines.h"

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "eve_expression_state.h"
#include "eve_output_ids.h"

// -----------------------------------------------------------------------------
// Step tables
// A step fires once when the routine has been running for atMs.
// -1 / 0 means "leave unchanged" for that output.
// -----------------------------------------------------------------------------
namespace {

struct Step {
  uint16_t atMs;
  int8_t expr;     /* EveExpressionId or -1 */
  uint16_t holdMs; /* expression hold */
  int8_t glow;     /* EveGlowPattern or -1 */
  uint8_t sound;   /* track number or 0 */
  int16_t head;    /* degrees or -1 */
  int16_t arm;     /* degrees or -1 */
  uint16_t dps;    /* servo speed, degrees per second */
};

struct Routine {
  const char* name;
  const Step* steps;
  uint8_t count;
  uint16_t endMs; /* routine finishes (glow overlay released) at this time */
  uint8_t priority;
  bool operatorAllowed;
  bool theatrical;
};

const int16_t H0 = EVE_SERVO_HEAD_REST_DEG;
const int16_t A0 = EVE_SERVO_ARM_REST_DEG;

/* Link established: wake — fade in, blink, soften. EVE's warmer second response. */
const Step kWake[] = {
    {0, EVE_EXPR_WAKE, 1400, EVE_GLOW_LINKED, 0, H0, -1, 45},
    {350, -1, 0, -1, EVE_SND_WAKE, -1, -1, 0},
    {1400, EVE_EXPR_SOFT_IDLE, 900, -1, 0, -1, -1, 0},
    {2300, EVE_EXPR_AFFECTION, 1600, -1, 0, -1, -1, 0},
};
/* Link lost: no motion (the link may have dropped because of a fault). Confusion, then search. */
const Step kLost[] = {
    {0, EVE_EXPR_CONFUSED, 1600, EVE_GLOW_WAITING, 0, -1, -1, 0},
    {500, -1, 0, -1, EVE_SND_LOST, -1, -1, 0},
    {1600, EVE_EXPR_SEARCHING_FOR_WALLE, 2400, -1, 0, -1, -1, 0},
};
/* Escort: decisive — quick, precise, few sounds. */
const Step kEscort[] = {
    {0, EVE_EXPR_ALERT, 900, EVE_GLOW_ESCORT, 0, H0, A0, 120},
    {900, EVE_EXPR_NEUTRAL_IDLE, 600, -1, 0, -1, -1, 0},
};
/* Greet / interaction: a pause, then softening — the slower second response. */
const Step kGreet[] = {
    {0, EVE_EXPR_SOFT_IDLE, 500, EVE_GLOW_INTERACT, 0, -1, -1, 0},
    {500, EVE_EXPR_HAPPY, 1500, -1, EVE_SND_GREET, H0, EVE_SERVO_ARM_GREET_DEG, 60},
    {2000, EVE_EXPR_AFFECTION, 1500, -1, 0, -1, -1, 0},
    {3500, -1, 0, -1, 0, -1, A0, 45},
};
/* Standby: settle slowly, close the eyes, beacon. */
const Step kStandby[] = {
    {0, EVE_EXPR_SLEEPY, 1500, EVE_GLOW_STANDBY_BEACON, EVE_SND_STANDBY, H0, A0, 30},
    {1500, EVE_EXPR_SLEEP, 4000, -1, 0, -1, -1, 0},
};
/* Verified critical battery: dim and still (motion is inhibited by the safety layer). */
const Step kLowPower[] = {
    {0, EVE_EXPR_LOW_BATTERY, 4000, EVE_GLOW_LOW_POWER, EVE_SND_LOW_POWER, -1, -1, 0},
};
/* Verified low battery warning: a single reaction, then back to normal. */
const Step kLowWarn[] = {
    {0, EVE_EXPR_LOW_BATTERY, 2500, EVE_GLOW_LOW_POWER, 0, -1, -1, 0},
};
/* Measured full charge. */
const Step kFull[] = {
    {0, EVE_EXPR_HAPPY, 1500, EVE_GLOW_FULL, 0, -1, -1, 0},
};
/* Operator scan: ANIMATION ONLY — glance left, right, centre. No detection happens. */
const Step kScan[] = {
    {0, EVE_EXPR_LOOK_LEFT, 800, EVE_GLOW_SCAN, 0, (int16_t)(H0 - EVE_SCAN_SWEEP_DEG), -1, 50},
    {800, EVE_EXPR_LOOK_RIGHT, 1000, -1, 0, (int16_t)(H0 + EVE_SCAN_SWEEP_DEG), -1, 50},
    {1800, EVE_EXPR_CURIOUS, 800, -1, EVE_SND_SCAN, H0, -1, 50},
    {2600, EVE_EXPR_NEUTRAL_IDLE, 400, -1, 0, -1, -1, 0},
};
/* Operator play: a short burst, still smooth. Poses match the existing docked-web "play". */
const Step kPlay[] = {
    {0, EVE_EXPR_HAPPY, 1200, EVE_GLOW_PLAY, EVE_SND_PLAY, 110, 150, 90},
    {1200, -1, 0, -1, 0, 75, 120, 90},
    {2000, EVE_EXPR_AFFECTION, 1200, -1, 0, H0, A0, 60},
};
/* Operator protect: steady attention, arm to the existing "attention" pose. A display, not detection. */
const Step kProtect[] = {
    {0, EVE_EXPR_ALERT, 2600, EVE_GLOW_PROTECT, EVE_SND_PROTECT, H0, EVE_SERVO_ARM_PROTECT_DEG, 60},
    {2600, EVE_EXPR_NEUTRAL_IDLE, 600, -1, 0, -1, A0, 45},
};

#define N(a) (uint8_t)(sizeof(a) / sizeof((a)[0]))
const Routine kRoutines[EVE_ROUTINE_COUNT] = {
    {"none", nullptr, 0, 0, 0, false, false},
    {"wake", kWake, N(kWake), 3900, 2, true, false},
    {"lost", kLost, N(kLost), 4000, 3, false, false},
    {"escort", kEscort, N(kEscort), 1500, 1, false, false},
    {"greet", kGreet, N(kGreet), 4000, 1, true, false},
    {"standby", kStandby, N(kStandby), 5500, 2, false, false},
    {"low_power", kLowPower, N(kLowPower), 4000, 3, false, false},
    {"low_warn", kLowWarn, N(kLowWarn), 2500, 2, false, false},
    {"full", kFull, N(kFull), 1500, 1, false, false},
    {"scan", kScan, N(kScan), 3000, 1, true, true},
    {"play", kPlay, N(kPlay), 3200, 1, true, false},
    {"protect", kProtect, N(kProtect), 3200, 1, true, false},
};
#undef N

EveRoutineHooks g_hooks = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
EveRoutineId g_active = EVE_ROUTINE_NONE;
uint32_t g_startMs = 0;
uint8_t g_next = 0;
uint32_t g_starts[EVE_ROUTINE_COUNT] = {0};

void logLine(const char* s) {
  if (g_hooks.log) g_hooks.log(s);
}

void applyStep(const Step& s) {
  if (s.expr >= 0 && g_hooks.expression) g_hooks.expression((uint8_t)s.expr, s.holdMs);
  if (s.glow >= 0 && g_hooks.glow) g_hooks.glow((uint8_t)s.glow, 0);
  if (s.sound != 0 && g_hooks.sound) g_hooks.sound(s.sound);
  if (g_hooks.servo) {
    if (s.head >= 0) g_hooks.servo(EVE_SERVO_HEAD_PAN, s.head, s.dps);
    if (s.arm >= 0) g_hooks.servo(EVE_SERVO_RIGHT_ARM, s.arm, s.dps);
  }
}

void finish(const char* why) {
  if (g_active == EVE_ROUTINE_NONE) return;
  char buf[64];
  snprintf(buf, sizeof(buf), "[EVE][ROUTINE] %s %s", kRoutines[g_active].name, why);
  logLine(buf);
  g_active = EVE_ROUTINE_NONE;
  if (g_hooks.glow) g_hooks.glow(0xFF, 0); /* 0xFF = release overlay */
}

}  // namespace

void eveRoutinesInit(const EveRoutineHooks* hooks) {
  if (hooks) g_hooks = *hooks;
  g_active = EVE_ROUTINE_NONE;
  g_next = 0;
}

bool eveRoutineStart(EveRoutineId id, uint32_t nowMs) {
  if (id <= EVE_ROUTINE_NONE || id >= EVE_ROUTINE_COUNT) return false;
  if (g_hooks.routinesAllowed && !g_hooks.routinesAllowed()) return false;
  if (g_active == id) return false; /* run once per event: never restart a running routine */
  if (g_active != EVE_ROUTINE_NONE && kRoutines[g_active].priority > kRoutines[id].priority) return false;
  if (g_active != EVE_ROUTINE_NONE) finish("replaced");
  g_active = id;
  g_startMs = nowMs;
  g_next = 0;
  g_starts[id]++;
  char buf[64];
  snprintf(buf, sizeof(buf), "[EVE][ROUTINE] start %s%s", kRoutines[id].name,
           kRoutines[id].theatrical ? " (animation only)" : "");
  logLine(buf);
  eveRoutinesTick(nowMs); /* fire the t=0 step immediately */
  return true;
}

void eveRoutineCancel(void) {
  finish("cancelled");
}

void eveRoutinesTick(uint32_t nowMs) {
  if (g_active == EVE_ROUTINE_NONE) return;
  if (g_hooks.routinesAllowed && !g_hooks.routinesAllowed()) {
    finish("cancelled (not allowed)");
    return;
  }
  const Routine& r = kRoutines[g_active];
  const uint32_t elapsed = nowMs - g_startMs;
  while (g_next < r.count && elapsed >= r.steps[g_next].atMs) {
    applyStep(r.steps[g_next]);
    g_next++;
    if (g_active == EVE_ROUTINE_NONE) return; /* a hook cancelled us */
  }
  if (g_next >= r.count && elapsed >= r.endMs) finish("done");
}

EveRoutineId eveRoutineActive(void) { return g_active; }

const char* eveRoutineName(EveRoutineId id) {
  return (id < EVE_ROUTINE_COUNT) ? kRoutines[id].name : "unknown";
}

EveRoutineId eveRoutineFromName(const char* name) {
  if (!name) return EVE_ROUTINE_NONE;
  for (uint8_t i = 1; i < EVE_ROUTINE_COUNT; i++) {
    if (strcmp(name, kRoutines[i].name) == 0) return (EveRoutineId)i;
  }
  return EVE_ROUTINE_NONE;
}

bool eveRoutineOperatorAllowed(EveRoutineId id) {
  return id > EVE_ROUTINE_NONE && id < EVE_ROUTINE_COUNT && kRoutines[id].operatorAllowed;
}

bool eveRoutineIsTheatrical(EveRoutineId id) {
  return id > EVE_ROUTINE_NONE && id < EVE_ROUTINE_COUNT && kRoutines[id].theatrical;
}

uint32_t eveRoutineStartCount(EveRoutineId id) {
  return (id < EVE_ROUTINE_COUNT) ? g_starts[id] : 0;
}
