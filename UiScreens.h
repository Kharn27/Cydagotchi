#pragma once

#include "UiLayout.h"

void drawTitleScreen();
void drawMenuScreen();
void drawNewPetScreen();
void drawGameScreenStatic();
void drawTopMenuBar();
void drawBottomMenuBar();
void drawGameViewMain(bool headerDirty, bool faceDirty, bool forceClear);
void drawGameViewStats(bool statsDirty);
void drawGameViewEatMenu(bool dirty);
void drawGameViewPlayMenu(bool dirty);
void drawGameViewWorldMenu(bool dirty);
void drawGameViewToiletMenu(bool dirty);
void drawGameViewDuelMenu(bool dirty);
void drawAlertIcon();
void drawGameScreenDynamic();
void drawGameScreen();

