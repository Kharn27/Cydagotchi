#include "GameState.h"

AppState appState = STATE_TITLE;
GameView currentGameView = VIEW_GAME;

unsigned long lastGameTickMillis = 0;
unsigned long lastAutoActionMillis = 0;
unsigned long lastRedrawMillis = 0;
unsigned long lastAutoSaveMillis = 0;
