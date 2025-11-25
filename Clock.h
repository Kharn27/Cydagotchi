#pragma once

#include <Arduino.h>

// Accelerated in-game clock: 1 minute IRL = 24 hours in-game.
// The helpers below expose the accelerated time for UI rendering and scheduling.

void clockInit();                       // Initialize reference start time (call from setup)
void clockReset();                      // Reset the in-game clock to 00:00
unsigned long getGameMinutesSinceStart();
void getGameTime(int &hours, int &minutes);  // hours 0-23, minutes 0-59

