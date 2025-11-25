#include "GameState.h"

AppState appState = STATE_TITLE;
GameView currentGameView = VIEW_MAIN;

unsigned long lastGameTickMillis = 0;
unsigned long lastAutoActionMillis = 0;
unsigned long lastRedrawMillis = 0;
unsigned long lastAutoSaveMillis = 0;
unsigned long gameClockStartMillis = 0;

void getGameTime(int &hours, int &minutes) {
  unsigned long realElapsed = millis() - gameClockStartMillis;
  unsigned long gameMinutes = (realElapsed * GAME_MINUTES_PER_DAY) / REAL_MS_PER_GAME_DAY;

  hours = (gameMinutes / 60) % 24;
  minutes = gameMinutes % 60;
}
