// ============================================================
//  WALL-E Waypoint Navigation System
//  GPS-based waypoint management and navigation
// ============================================================

#ifndef WAYPOINT_NAV_H
#define WAYPOINT_NAV_H

#include <Arduino.h>

// Maximum number of waypoints
#define MAX_WAYPOINTS  10

// Waypoint arrival threshold (meters)
#define WAYPOINT_ARRIVAL_RADIUS  2.0f

// Waypoint structure
struct Waypoint {
  double latitude;
  double longitude;
  char name[16];
  bool active;
};

// Navigation state
struct NavState {
  uint8_t currentWaypointIndex;
  float distanceToWaypoint;      // meters
  float bearingToWaypoint;       // degrees (0-360)
  bool waypointActive;
  bool navigationActive;
};

// Initialize waypoint system
void waypointInit();

// Add waypoint
bool waypointAdd(double lat, double lon, const char* name);

// Remove waypoint
bool waypointRemove(uint8_t index);

// Clear all waypoints
void waypointClearAll();

// Get waypoint by index
const Waypoint* waypointGet(uint8_t index);

// Get total waypoint count
uint8_t waypointGetCount();

// Set current waypoint (target)
void waypointSetCurrent(uint8_t index);

// Get current waypoint index
uint8_t waypointGetCurrent();

// Update navigation (call every loop with current GPS position)
void waypointUpdate(double currentLat, double currentLon, float currentHeading);

// Get navigation state
const NavState* waypointGetNavState();

// Start/stop navigation
void waypointStartNavigation();
void waypointStopNavigation();
bool waypointIsNavigating();

// Check if waypoint reached
bool waypointIsArrived();

// Move to next waypoint in sequence
void waypointNext();

// Calculate distance between two GPS coordinates (Haversine formula)
float waypointCalculateDistance(double lat1, double lon1, double lat2, double lon2);

// Calculate bearing from point 1 to point 2 (degrees, 0=North)
float waypointCalculateBearing(double lat1, double lon1, double lat2, double lon2);

// Save waypoints to flash (Preferences)
void waypointSave();

// Load waypoints from flash
void waypointLoad();

#endif // WAYPOINT_NAV_H
