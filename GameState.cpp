#include "GameState.h"

AppState appState = STATE_TITLE;
GameView currentGameView = VIEW_MAIN;

unsigned long lastGameTickMillis = 0;
unsigned long lastAutoActionMillis = 0;
unsigned long lastRedrawMillis = 0;
unsigned long lastAutoSaveMillis = 0;
float gameMinutesSinceMidnight = 0.0f;

void resetGameClock() {
  gameMinutesSinceMidnight = 0.0f;
}

void advanceGameClock(float dtSeconds) {
  gameMinutesSinceMidnight += dtSeconds * GAME_MINUTES_PER_REAL_SECOND;

  while (gameMinutesSinceMidnight >= static_cast<float>(GAME_MINUTES_PER_DAY)) {
    gameMinutesSinceMidnight -= static_cast<float>(GAME_MINUTES_PER_DAY);
  }
}

void getGameTime(int &hours, int &minutes) {
  unsigned long wholeMinutes = static_cast<unsigned long>(gameMinutesSinceMidnight) % GAME_MINUTES_PER_DAY;
  hours = static_cast<int>(wholeMinutes / 60UL);
  minutes = static_cast<int>(wholeMinutes % 60UL);
}
