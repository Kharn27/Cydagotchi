#pragma once

#include <Arduino.h>
#include "UiLayout.h"

extern AppState appState;
extern GameView currentGameView;

extern unsigned long lastGameTickMillis;
extern unsigned long lastAutoActionMillis;
extern unsigned long lastRedrawMillis;
extern unsigned long lastAutoSaveMillis;

// Accelerated in-game clock (1 minute IRL = 1 day in-game)
constexpr unsigned long REAL_MS_PER_GAME_DAY = 60000UL;
constexpr unsigned long GAME_MINUTES_PER_DAY = 24UL * 60UL;
extern unsigned long gameClockStartMillis;

void getGameTime(int &hours, int &minutes);
