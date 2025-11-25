#pragma once

#include <Arduino.h>

// Accelerated in-game clock: 1 minute IRL = 24 hours in-game.
// The helpers below expose the accelerated time for UI rendering and scheduling.

enum TimeOfDaySlot {
  SLOT_NIGHT = 0,
  SLOT_DAWN = 1,
  SLOT_DAY = 2,
  SLOT_EVENING = 3,
  SLOT_STORM = 4
};

void clockInit();                       // Initialize reference start time (call from setup)
void clockReset();                      // Reset the in-game clock to 00:00
unsigned long getGameMinutesSinceStart();
void getGameTime(int &hours, int &minutes);  // hours 0-23, minutes 0-59
TimeOfDaySlot getTimeOfDaySlot();            // Discretized time-of-day slot (night/dawn/day/...)

