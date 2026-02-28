// ============================================================
//  WALL-E Master Controller — Circular Button UI
//  8-direction buttons arranged in a circle
// ============================================================

#ifndef UI_BUTTONS_H
#define UI_BUTTONS_H

#include <stdint.h>

// Button layout: 8 directions around center
#define BTN_CENTER_X  160
#define BTN_CENTER_Y  126
#define BTN_RADIUS    70   // Distance from center to buttons
#define BTN_SIZE      40   // Button width/height
#define BTN_INNER_R   30   // Dead zone radius

// Button directions (8 directions)
typedef enum {
  DIR_NONE = -1,
  DIR_UP = 0,        // North
  DIR_UP_RIGHT,      // NE
  DIR_RIGHT,         // East
  DIR_DOWN_RIGHT,    // SE
  DIR_DOWN,          // South
  DIR_DOWN_LEFT,     // SW
  DIR_LEFT,          // West
  DIR_UP_LEFT,       // NW
  DIR_COUNT = 8
} Direction;

// Get button region for touch
Direction getDirectionFromTouch(int touchX, int touchY);

// Get motor speeds from direction
void getSpeedsFromDirection(Direction dir, int8_t* outLeft, int8_t* outRight);

#endif // UI_BUTTONS_H
