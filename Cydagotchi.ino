// Bibliothèques principales
#include <SPI.h>
#include <cstring>
#include <TFT_eSPI.h>
// #include <XPT2046_Touchscreen.h>
#include <XPT2046_Touchscreen_TT.h>
#include <SPIFFS.h>

#include "PetModel.h"
#include "PetLogic.h"
#include "PetStorage.h"
#include "UiLayout.h"
#include "UiScreens.h"
#include "Input.h"
#include "Actions.h"
#include "GameState.h"
#include "GameLoop.h"
#include "Clock.h"

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

TFT_eSPI tft = TFT_eSPI();

// SPI séparé pour le tactile
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// Journal d'action (dernière action effectuée) dans Actions.cpp
// Sélection de personnalité et nom pour NEW_PET dans Actions.cpp

// --- Définition des boutons par scène ---
Button menuButtons[] = {
  { 60, 110, 200, 40, "Nouveau Cydagotchi", TFT_DARKGREEN, TFT_WHITE, TFT_WHITE, actionStartNewPet },
  { 60, 160, 200, 40, "Load Pet", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionLoadPet }
};

Button newPetButtons[] = {
  // Personnalité
  { 40, 140, 50, 30, "<P",  TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionPrevPersonality },
  { 230, 140, 50, 30, "P>", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionNextPersonality },

  // Nom
  { 40, 175, 50, 30, "<N",  TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionPrevName },
  { 230, 175, 50, 30, "N>", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionNextName },

  // Démarrer le jeu
  { 70, 210, 180, 30, "Demarrer le jeu", TFT_GREEN, TFT_WHITE, TFT_WHITE, actionStartGameFromNewPet }
};

const int16_t topMenuButtonWidth = (SCREEN_W - TOP_CLOCK_AREA_W) / TOPMENU_COUNT;
Button topMenuButtons[] = {
  { topMenuButtonWidth * TOPMENU_STATS, 0, topMenuButtonWidth, TOP_MENU_HEIGHT, "Stats", TFT_DARKCYAN, TFT_WHITE, HUD_BORDER_COLOR, actionShowStats, TOPMENU_STATS },
  { topMenuButtonWidth * TOPMENU_EAT, 0, topMenuButtonWidth, TOP_MENU_HEIGHT, "Manger", TFT_DARKGREEN, TFT_WHITE, HUD_BORDER_COLOR, actionShowFeed, TOPMENU_EAT },
  { topMenuButtonWidth * TOPMENU_PLAY, 0, topMenuButtonWidth, TOP_MENU_HEIGHT, "Jeu", TFT_BLUE, TFT_WHITE, HUD_BORDER_COLOR, actionShowPlayMenu, TOPMENU_PLAY },
  { topMenuButtonWidth * TOPMENU_WORLD, 0, topMenuButtonWidth, TOP_MENU_HEIGHT, "Monde", TFT_MAGENTA, TFT_WHITE, HUD_BORDER_COLOR, actionShowWorldMenu, TOPMENU_WORLD }
};

static_assert(TOPMENU_COUNT == (sizeof(topMenuButtons) / sizeof(topMenuButtons[0])),
              "Top menu button array size must match enum indices");

const int16_t bottomButtonWidth = (SCREEN_W - ALERT_AREA_W) / 3;
Button bottomMenuButtons[] = {
  { 0, ALERT_AREA_Y, bottomButtonWidth, BOTTOM_MENU_HEIGHT, "Lumiere", TFT_DARKGREY, TFT_WHITE, HUD_BORDER_COLOR, actionToggleLights, BOTTOMMENU_LIGHTS },
  { bottomButtonWidth, ALERT_AREA_Y, bottomButtonWidth, BOTTOM_MENU_HEIGHT, "Toilette", TFT_CYAN, TFT_BLACK, HUD_BORDER_COLOR, actionShowToiletMenu, BOTTOMMENU_TOILET },
  { bottomButtonWidth * 2, ALERT_AREA_Y, bottomButtonWidth, BOTTOM_MENU_HEIGHT, "Duel", TFT_RED, TFT_WHITE, HUD_BORDER_COLOR, actionShowDuelMenu, BOTTOMMENU_DUEL }
};

extern const size_t MENU_BUTTON_COUNT       = sizeof(menuButtons) / sizeof(menuButtons[0]);
extern const size_t NEWPET_BUTTON_COUNT     = sizeof(newPetButtons) / sizeof(newPetButtons[0]);
extern const size_t TOPMENU_BUTTON_COUNT    = TOPMENU_COUNT;
extern const size_t BOTTOMMENU_BUTTON_COUNT = sizeof(bottomMenuButtons) / sizeof(bottomMenuButtons[0]);
extern const size_t FEED_MENU_BUTTON_COUNT  = 3;
extern const size_t PLAY_MENU_BUTTON_COUNT  = 3;
extern const size_t TOILET_MENU_BUTTON_COUNT = 2;

Button feedMenuButtons[FEED_MENU_BUTTON_COUNT] = {
  { 16, TOP_MENU_HEIGHT + 70, 130, 36, "Croquette", TFT_DARKGREEN, TFT_WHITE, HUD_BORDER_COLOR, actionEat },
  { SCREEN_W - 146, TOP_MENU_HEIGHT + 70, 130, 36, "Snack", TFT_GREEN, TFT_WHITE, HUD_BORDER_COLOR, actionEat },
  { 16, TOP_MENU_HEIGHT + 112, 260, 36, "Repas complet", TFT_DARKGREY, TFT_WHITE, HUD_BORDER_COLOR, actionEat }
};

Button playMenuButtons[PLAY_MENU_BUTTON_COUNT] = {
  { 16, TOP_MENU_HEIGHT + 70, 130, 36, "Balle", TFT_BLUE, TFT_WHITE, HUD_BORDER_COLOR, actionPlay },
  { SCREEN_W - 146, TOP_MENU_HEIGHT + 70, 130, 36, "Console", TFT_NAVY, TFT_WHITE, HUD_BORDER_COLOR, actionPlay },
  { 16, TOP_MENU_HEIGHT + 112, 260, 36, "Mini-jeu", TFT_DARKCYAN, TFT_WHITE, HUD_BORDER_COLOR, actionPlay }
};

Button toiletMenuButtons[TOILET_MENU_BUTTON_COUNT] = {
  { 16, TOP_MENU_HEIGHT + 70, 130, 36, "Douche", TFT_CYAN, TFT_BLACK, HUD_BORDER_COLOR, actionWash },
  { SCREEN_W - 146, TOP_MENU_HEIGHT + 70, 130, 36, "Peigne", TFT_LIGHTGREY, TFT_BLACK, HUD_BORDER_COLOR, actionWash }
};

void changeScene(AppState next) {
  appState = next;

  switch (appState) {
    case STATE_TITLE:
      drawTitleScreen();
      break;
    case STATE_MENU:
      drawMenuScreen();
      break;
    case STATE_NEW_PET:
      if (!hasNewPetPersonality) {
        newPetPersonality = PERSO_EQUILIBRE;
        hasNewPetPersonality = true;
      }

      if (!hasNewPetName) {
        newPetNameIndex = 0;
        strncpy(newPetName, PRESET_NAMES[newPetNameIndex], sizeof(newPetName));
        newPetName[sizeof(newPetName) - 1] = '\0';
        hasNewPetName = true;
      }
      drawNewPetScreen();
      break;
    case STATE_GAME:
      currentGameView = VIEW_MAIN;
      resetGameScreenCache();
      if (!petInitialized) {
        if (hasNewPetPersonality && hasNewPetName) {
          initPetWithPersonality(newPetPersonality, newPetName);
        } else if (hasNewPetPersonality) {
          initPetWithPersonality(newPetPersonality, "Cydy");
        } else {
          initDefaultPet();
        }
        char birthMsg[32];
        snprintf(birthMsg, sizeof(birthMsg), "%s vient de naitre", currentPet.name);
        setLastAction(birthMsg, false);
      }
      lastGameTickMillis = millis();
      clockReset();
      lastAutoActionMillis = lastGameTickMillis;
      lastRedrawMillis = lastGameTickMillis;
      lastAutoSaveMillis = lastGameTickMillis;
      drawGameScreenStatic();
      drawGameScreenDynamic();
      break;
  }
}

void setup() {
  Serial.begin(115200);

  petStorageBegin();

  if (!SPIFFS.begin(true)) {
    Serial.println("[FS] SPIFFS mount failed");
  }

  randomSeed(analogRead(34));

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(1);

  tft.init();
  tft.setRotation(1);

  lightsOff = false;
  tft.invertDisplay(false);

  clockInit();

  changeScene(STATE_TITLE);
}

void loop() {
  switch (appState) {
    case STATE_TITLE:
      {
        int16_t x, y;
        if (getTouch(x, y)) {
          delay(150);
          while (ts.touched()) {}
          changeScene(STATE_MENU);
        }
      }
      break;

    case STATE_MENU:
      processTouchForButtons(menuButtons, MENU_BUTTON_COUNT);
      break;

    case STATE_NEW_PET:
      processTouchForButtons(newPetButtons, NEWPET_BUTTON_COUNT);
      break;

    case STATE_GAME:
      gameLoopTick();
      break;
  }
}

