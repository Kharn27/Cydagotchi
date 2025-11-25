#include "Clock.h"

namespace {
// Using milliseconds as the base avoids floating-point drift while keeping the math simple.
unsigned long startMillis = 0;
constexpr unsigned long MINUTES_PER_DAY = 24UL * 60UL;
constexpr unsigned long GAME_MINUTES_PER_REAL_SECOND = 24UL;  // 1 sec IRL = 24 min in-game
}  // namespace

void clockInit() {
  startMillis = millis();
}

void clockReset() {
  startMillis = millis();
}

unsigned long getGameMinutesSinceStart() {
  unsigned long elapsedMs = millis() - startMillis;
  unsigned long elapsedSec = elapsedMs / 1000UL;
  return elapsedSec * GAME_MINUTES_PER_REAL_SECOND;
}

void getGameTime(int &hours, int &minutes) {
  unsigned long totalMinutes = getGameMinutesSinceStart();
  unsigned long dayMinutes = totalMinutes % MINUTES_PER_DAY;
  hours = static_cast<int>(dayMinutes / 60UL);
  minutes = static_cast<int>(dayMinutes % 60UL);
}

TimeOfDaySlot getTimeOfDaySlot() {
  int hours = 0;
  int minutes = 0;
  getGameTime(hours, minutes);

  int totalMinutes = hours * 60 + minutes;
  int nightStart = 21 * 60 + 30;
  int dawnStart = 6 * 60 + 30;
  int dayStart = 8 * 60 + 30;
  int eveningStart = 21 * 60 + 30;

  if (totalMinutes >= nightStart || totalMinutes < dawnStart) return SLOT_NIGHT;
  if (totalMinutes < dayStart) return SLOT_DAWN;
  if (totalMinutes < eveningStart) return SLOT_DAY;
  return SLOT_EVENING;
}

