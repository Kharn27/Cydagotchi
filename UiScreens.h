#pragma once

#include "UiLayout.h"
#include "Actions.h"

void drawTitleScreen();
void drawMenuScreen();
void drawNewPetScreen();
void drawGameScreenStatic();
void drawGameViewMain(bool headerDirty, bool needsDirty, bool faceDirty);
void drawGameViewStats(bool statsDirty);
void drawAlertIcon();
void drawGameScreenDynamic();
void drawGameScreen();

