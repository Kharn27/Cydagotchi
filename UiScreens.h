#pragma once

#include "UiLayout.h"

void drawTitleScreen();
void drawMenuScreen();
void drawNewPetScreen();
void drawGameScreenStatic();
void drawTopMenuBar();
void drawGameViewMain(bool headerDirty, bool faceDirty, bool forceClear);
void drawGameViewStats(bool statsDirty);
void drawGameViewFeed(bool dirty);
void drawGameViewWorld(bool dirty);
void drawAlertIcon();
void drawGameScreenDynamic();
void drawGameScreen();

