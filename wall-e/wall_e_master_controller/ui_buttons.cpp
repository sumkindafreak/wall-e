// ============================================================
//  WALL-E Master Controller — Circular Button Implementation
// ============================================================

#include "ui_buttons.h"
#include <math.h>

// Speed for each direction (8-way movement)
// Format: {leftSpeed, rightSpeed} - SWAPPED for physical motor wiring
static const int8_t s_directionSpeeds[DIR_COUNT][2] = {
  {100, 100},   // UP: Forward
  {50, 100},    // UP_RIGHT: Forward + turn right (SWAPPED)
  {-100, 100},  // RIGHT: Spin right (SWAPPED)
  {-100, 50},   // DOWN_RIGHT: Back + turn right (SWAPPED)
  {-100, -100}, // DOWN: Backward
  {-50, -100},  // DOWN_LEFT: Back + turn left (SWAPPED)
  {100, -100},  // LEFT: Spin left (SWAPPED)
  {100, -50}    // UP_LEFT: Forward + turn left (SWAPPED)
};

Direction getDirectionFromTouch(int touchX, int touchY) {
  int dx = touchX - BTN_CENTER_X;
  int dy = touchY - BTN_CENTER_Y;
  
  float distance = sqrtf(dx * dx + dy * dy);
  
  // Inside dead zone = no input
  if (distance < BTN_INNER_R) {
    return DIR_NONE;
  }
  
  // Calculate angle: 0° = right, 90° = down, 180° = left, 270° = up
  float angle = atan2f(dy, dx) * 180.0f / M_PI;
  
  // Normalize to 0-360
  if (angle < 0) angle += 360.0f;
  
  // Convert angle to direction (8 sectors, 45° each)
  // Rotate by 90° so 0° = up
  angle = angle + 90.0f;
  if (angle >= 360.0f) angle -= 360.0f;
  
  // Map to 8 directions
  int sector = (int)((angle + 22.5f) / 45.0f) % 8;
  return (Direction)sector;
}

void getSpeedsFromDirection(Direction dir, int8_t* outLeft, int8_t* outRight) {
  if (dir == DIR_NONE || dir < 0 || dir >= DIR_COUNT) {
    *outLeft = 0;
    *outRight = 0;
    return;
  }
  
  *outLeft = s_directionSpeeds[dir][0];
  *outRight = s_directionSpeeds[dir][1];
}
