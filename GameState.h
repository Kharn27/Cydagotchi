#pragma once

#include <Arduino.h>
#include "UiLayout.h"

extern AppState appState;
extern GameView currentGameView;

extern unsigned long lastGameTickMillis;
extern unsigned long lastAutoActionMillis;
extern unsigned long lastRedrawMillis;
extern unsigned long lastAutoSaveMillis;
