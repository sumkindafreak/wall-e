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
//  Animation Library
// ============================================================
const AnimationDef animationLibrary[] PROGMEM = {
  {"Reset",         anim0_frames, sizeof(anim0_frames)/sizeof(AnimationFrame)},
  {"Bootup",        anim1_frames, sizeof(anim1_frames)/sizeof(AnimationFrame)},
  {"Inquisitive",   anim2_frames, sizeof(anim2_frames)/sizeof(AnimationFrame)},
  {"EyebrowRight",  anim3_frames, sizeof(anim3_frames)/sizeof(AnimationFrame)},
  {"EyebrowLeft",   anim4_frames, sizeof(anim4_frames)/sizeof(AnimationFrame)},
  {"Surprised",     anim5_frames, sizeof(anim5_frames)/sizeof(AnimationFrame)}
};

#define ANIMATION_COUNT (sizeof(animationLibrary)/sizeof(AnimationDef))

#endif // ANIMATION_DATA_H
