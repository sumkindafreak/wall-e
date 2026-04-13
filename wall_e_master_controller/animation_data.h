// ============================================================
//  WALL-E Master Controller — Animation Data
//  Simon Bluett's original animation format adapted
//  Now includes eyebrow servos (left/right)
// ============================================================

#ifndef ANIMATION_DATA_H
#define ANIMATION_DATA_H

#include <Arduino.h>

// Animation keyframe structure
struct AnimationFrame {
  uint16_t timeMs;      // Hold time in milliseconds
  int8_t head;          // 0-100 or -1 (disabled)
  int8_t neckTop;       // 0-100 or -1
  int8_t neckBottom;    // 0-100 or -1
  int8_t eyeRight;      // 0-100 or -1
  int8_t eyeLeft;       // 0-100 or -1
  int8_t armLeft;       // 0-100 or -1
  int8_t armRight;      // 0-100 or -1
  int8_t eyebrowRight;  // 0-100 or -1 (NEW)
  int8_t eyebrowLeft;   // 0-100 or -1 (NEW)
};

// Animation definition
struct AnimationDef {
  const char* name;
  const AnimationFrame* frames;
  uint8_t frameCount;
};

// ============================================================
//  ANIMATION 0: Reset/Neutral
// ============================================================
const AnimationFrame anim0_frames[] PROGMEM = {
  // time, head, nT, nB, eyeR, eyeL, armL, armR, ebrR, ebrL
  {1000,   50, 10,  0,   0,    0,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 1: Bootup Eye Sequence
// ============================================================
const AnimationFrame anim1_frames[] PROGMEM = {
  // time, head, nT, nB, eyeR, eyeL, armL, armR, ebrR, ebrL
  {2000,   50, 45, 90,  40,   40,   40,   40,    0,    0},
  { 700,   50, 45, 90,  40,    0,   40,   40,    0,    0},
  { 700,   50, 45, 90,   0,    0,   40,   40,    0,    0},
  { 700,   50, 45, 90,   0,   40,   40,   40,    0,    0},
  { 700,   50, 45, 90,  40,   40,   40,   40,    0,    0},
  { 400,   50, 45, 90,   0,    0,   40,   40,    0,    0},
  { 400,   50, 45, 90,  40,   40,   40,   40,    0,    0},
  {2000,   50,  0, 60,  40,   40,   40,   40,    0,    0},
  {1000,   50,  0, 60,   0,    0,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 2: Inquisitive Motion Sequence
// ============================================================
const AnimationFrame anim2_frames[] PROGMEM = {
  // time, head, nT, nB, eyeR, eyeL, armL, armR, ebrR, ebrL
  {3000,   48, 40,  0,  35,   45,   60,   59,    0,    0},
  {1500,   48, 40, 20, 100,    0,   80,   80,    0,    0},
  {3000,    0, 40, 40, 100,    0,   80,   80,    0,    0},
  {1500,   48, 60,100,  40,   40,  100,  100,    0,    0},
  {1500,   48, 40, 30,  45,   35,    0,    0,    0,    0},
  {1500,   34, 34, 10,  14,  100,    0,    0,    0,    0},
  {1500,   48, 60, 20,  35,   45,   60,   59,    0,    0},
  {3000,  100, 20, 50,  40,   40,   60,  100,    0,    0},
  {1500,   48, 15,  0,   0,    0,    0,    0,    0,    0},
  {1000,   50, 10,  0,   0,    0,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 3: Eyebrow Raise Right (NEW)
// ============================================================
const AnimationFrame anim3_frames[] PROGMEM = {
  // time, head, nT, nB, eyeR, eyeL, armL, armR, ebrR, ebrL
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,  100,   -1},  // Raise right eyebrow
  { 500,   -1, -1, -1,  -1,   -1,   -1,   -1,  100,   -1},  // Hold
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,    0,   -1}   // Lower
};

// ============================================================
//  ANIMATION 4: Eyebrow Raise Left (NEW)
// ============================================================
const AnimationFrame anim4_frames[] PROGMEM = {
  // time, head, nT, nB, eyeR, eyeL, armL, armR, ebrR, ebrL
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,   -1,  100},  // Raise left eyebrow
  { 500,   -1, -1, -1,  -1,   -1,   -1,   -1,   -1,  100},  // Hold
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,   -1,    0}   // Lower
};

// ============================================================
//  ANIMATION 5: Surprised (Both Eyebrows) (NEW)
// ============================================================
const AnimationFrame anim5_frames[] PROGMEM = {
  // time, head, nT, nB, eyeR, eyeL, armL, armR, ebrR, ebrL
  { 150,   -1, -1, -1, 100,  100,   -1,   -1,  100,  100},  // Eyes + eyebrows wide
  { 800,   -1, -1, -1, 100,  100,   -1,   -1,  100,  100},  // Hold
  { 200,   -1, -1, -1,  40,   40,   -1,   -1,    0,    0}   // Return to normal
};

// ============================================================
//  ANIMATION 6: Nod (yes)
// ============================================================
const AnimationFrame anim6_frames[] PROGMEM = {
  { 400,   50, 30, 20,  40,   40,   40,   40,    0,    0},
  { 350,   50, 55, 35,  40,   40,   40,   40,    0,    0},
  { 350,   50, 30, 20,  40,   40,   40,   40,    0,    0},
  { 400,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 7: Look side (head pan sweep)
// ============================================================
const AnimationFrame anim7_frames[] PROGMEM = {
  { 500,   20, 25, 10,  35,   35,   45,   45,    0,    0},
  { 700,   85, 35, 15,  50,   50,   55,   55,    0,    0},
  { 500,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 8: Arm wave (simple)
// ============================================================
const AnimationFrame anim8_frames[] PROGMEM = {
  { 400,   50, 15,  0,  40,   40,   30,   70,    0,    0},
  { 400,   50, 15,  0,  40,   40,   70,   30,    0,    0},
  { 500,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 9: Sleepy / droop
// ============================================================
const AnimationFrame anim9_frames[] PROGMEM = {
  {1200,   50, 20,  5,  15,   15,   35,   35,    0,    0},
  { 800,   50, 10,  0,  10,   10,   40,   40,    0,    0},
  {1000,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 10: Shake no
// ============================================================
const AnimationFrame anim10_frames[] PROGMEM = {
  { 300,   35, 25, 10,  40,   40,   42,   42,    0,    0},
  { 280,   65, 25, 10,  40,   40,   42,   42,    0,    0},
  { 280,   35, 25, 10,  40,   40,   42,   42,    0,    0},
  { 280,   65, 25, 10,  40,   40,   42,   42,    0,    0},
  { 400,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 11: Happy perk / brighten
// ============================================================
const AnimationFrame anim11_frames[] PROGMEM = {
  { 400,   52, 35, 15,  70,   70,   55,   55,   20,   20},
  { 600,   50, 28, 10,  55,   55,   50,   50,    0,    0},
  { 500,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 12: Sad / heavy
// ============================================================
const AnimationFrame anim12_frames[] PROGMEM = {
  { 900,   45, 15,  5,  20,   20,   30,   30,    0,    0},
  {1200,   40, 10,  0,  12,   12,   28,   28,    0,    0},
  { 800,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 13: Confused tilt (head cock)
// ============================================================
const AnimationFrame anim13_frames[] PROGMEM = {
  { 600,   48, 40, 25,  50,   30,   45,   55,   40,    0},
  {1200,   48, 40, 25,  50,   30,   45,   55,   40,    0},
  { 500,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 14: Lean in (curious)
// ============================================================
const AnimationFrame anim14_frames[] PROGMEM = {
  { 500,   55, 50, 30,  55,   55,   50,   50,    0,    0},
  {1500,   58, 55, 35,  60,   60,   52,   52,    0,    0},
  { 600,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 15: Peek (cautious)
// ============================================================
const AnimationFrame anim15_frames[] PROGMEM = {
  { 400,   42, 22,  8,  25,   25,   35,   35,    0,    0},
  { 800,   48, 30, 12,  70,   70,   38,   38,    0,    0},
  { 400,   42, 22,  8,  25,   25,   35,   35,    0,    0},
  { 500,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 16: Stretch / yawn
// ============================================================
const AnimationFrame anim16_frames[] PROGMEM = {
  {1200,   48, 18,  5,  30,   30,   35,   35,    0,    0},
  { 800,   52, 65, 40,  45,   45,   60,   60,    0,    0},
  {1000,   50, 12,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 17: Startle / recoil
// ============================================================
const AnimationFrame anim17_frames[] PROGMEM = {
  { 120,   50, 10,  0,  40,   40,   40,   40,    0,    0},
  { 400,   25,  8,  0,  90,   90,   30,   30,  100,  100},
  { 700,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 18: Brow wiggle (playful)
// ============================================================
const AnimationFrame anim18_frames[] PROGMEM = {
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,  100,    0},
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,    0,  100},
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,  100,    0},
  { 200,   -1, -1, -1,  -1,   -1,   -1,   -1,    0,  100},
  { 300,   -1, -1, -1,  -1,   -1,   -1,   -1,    0,    0}
};

// ============================================================
//  ANIMATION 19: Celebrate (arms up)
// ============================================================
const AnimationFrame anim19_frames[] PROGMEM = {
  { 400,   50, 25, 10,  50,   50,   85,   85,    0,    0},
  {1200,   55, 30, 15,  60,   60,   95,   95,   50,   50},
  { 600,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 20: Shy (arms in, duck)
// ============================================================
const AnimationFrame anim20_frames[] PROGMEM = {
  { 800,   42, 25,  8,  22,   22,   25,   25,    0,    0},
  {1000,   38, 20,  5,  15,   15,   22,   22,    0,    0},
  { 700,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 21: Look up
// ============================================================
const AnimationFrame anim21_frames[] PROGMEM = {
  { 600,   52, 55, 45,  65,   65,   45,   45,    0,    0},
  {1200,   55, 60, 50,  80,   80,   42,   42,    0,    0},
  { 600,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 22: Look down (shy floor)
// ============================================================
const AnimationFrame anim22_frames[] PROGMEM = {
  { 800,   42, 12,  0,  18,   18,   38,   38,    0,    0},
  {1200,   38,  8,  0,  12,   12,   35,   35,    0,    0},
  { 700,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  ANIMATION 23: Idle fidget
// ============================================================
const AnimationFrame anim23_frames[] PROGMEM = {
  { 400,   48, 22,  8,  42,   45,   42,   44,    0,    0},
  { 350,   52, 24, 10,  45,   42,   44,   42,    0,    0},
  { 400,   46, 20,  7,  40,   43,   43,   45,    0,    0},
  { 500,   50, 10,  0,  40,   40,   40,   40,    0,    0}
};

// ============================================================
//  Animation Library
// ============================================================
const AnimationDef animationLibrary[] PROGMEM = {
  {"Reset",         anim0_frames, sizeof(anim0_frames)/sizeof(AnimationFrame)},
  {"Bootup",        anim1_frames, sizeof(anim1_frames)/sizeof(AnimationFrame)},
  {"Inquisitive",   anim2_frames, sizeof(anim2_frames)/sizeof(AnimationFrame)},
  {"EyebrowRight",  anim3_frames, sizeof(anim3_frames)/sizeof(AnimationFrame)},
  {"EyebrowLeft",   anim4_frames, sizeof(anim4_frames)/sizeof(AnimationFrame)},
  {"Surprised",     anim5_frames, sizeof(anim5_frames)/sizeof(AnimationFrame)},
  {"Nod",           anim6_frames, sizeof(anim6_frames)/sizeof(AnimationFrame)},
  {"LookSide",      anim7_frames, sizeof(anim7_frames)/sizeof(AnimationFrame)},
  {"Wave",          anim8_frames, sizeof(anim8_frames)/sizeof(AnimationFrame)},
  {"Sleepy",        anim9_frames, sizeof(anim9_frames)/sizeof(AnimationFrame)},
  {"ShakeNo",       anim10_frames, sizeof(anim10_frames)/sizeof(AnimationFrame)},
  {"HappyPerk",     anim11_frames, sizeof(anim11_frames)/sizeof(AnimationFrame)},
  {"SadHeavy",      anim12_frames, sizeof(anim12_frames)/sizeof(AnimationFrame)},
  {"ConfusedTilt",  anim13_frames, sizeof(anim13_frames)/sizeof(AnimationFrame)},
  {"LeanIn",        anim14_frames, sizeof(anim14_frames)/sizeof(AnimationFrame)},
  {"Peek",          anim15_frames, sizeof(anim15_frames)/sizeof(AnimationFrame)},
  {"YawnStretch",   anim16_frames, sizeof(anim16_frames)/sizeof(AnimationFrame)},
  {"Startle",       anim17_frames, sizeof(anim17_frames)/sizeof(AnimationFrame)},
  {"BrowWiggle",    anim18_frames, sizeof(anim18_frames)/sizeof(AnimationFrame)},
  {"Celebrate",     anim19_frames, sizeof(anim19_frames)/sizeof(AnimationFrame)},
  {"ShyDuck",       anim20_frames, sizeof(anim20_frames)/sizeof(AnimationFrame)},
  {"LookUp",        anim21_frames, sizeof(anim21_frames)/sizeof(AnimationFrame)},
  {"LookDown",      anim22_frames, sizeof(anim22_frames)/sizeof(AnimationFrame)},
  {"IdleFidget",    anim23_frames, sizeof(anim23_frames)/sizeof(AnimationFrame)}
};

#define ANIMATION_COUNT (sizeof(animationLibrary)/sizeof(AnimationDef))

#endif // ANIMATION_DATA_H
