#pragma once

#include <Arduino.h>
#include "UiLayout.h"

extern AppState appState;
extern GameView currentGameView;

extern unsigned long lastGameTickMillis;
extern unsigned long lastAutoActionMillis;
extern unsigned long lastRedrawMillis;
extern unsigned long lastAutoSaveMillis;

// Accelerated in-game clock (1 second IRL = 24 minutes in-game)
constexpr float GAME_MINUTES_PER_REAL_SECOND = 24.0f;
constexpr unsigned long GAME_MINUTES_PER_DAY = 24UL * 60UL;
extern float gameMinutesSinceMidnight;

void resetGameClock();
void advanceGameClock(float dtSeconds);
void getGameTime(int &hours, int &minutes);
