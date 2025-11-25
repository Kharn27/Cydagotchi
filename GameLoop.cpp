#include <Arduino.h>
#include "GameLoop.h"
#include "Actions.h"
#include "GameState.h"
#include "Input.h"
#include "PetLogic.h"
#include "UiScreens.h"

extern Button topMenuButtons[];
extern Button bottomMenuButtons[];
extern Button feedMenuButtons[];
extern Button playMenuButtons[];
extern Button toiletMenuButtons[];
extern const size_t TOPMENU_BUTTON_COUNT;
extern const size_t BOTTOMMENU_BUTTON_COUNT;
extern const size_t FEED_MENU_BUTTON_COUNT;
extern const size_t PLAY_MENU_BUTTON_COUNT;
extern const size_t TOILET_MENU_BUTTON_COUNT;

static const unsigned long GAME_TICK_INTERVAL_MS = 400;
static const unsigned long AUTO_ACTION_INTERVAL_MS = 6000;
static const unsigned long GAME_REDRAW_INTERVAL_MS = 1000;
static const unsigned long AUTO_SAVE_INTERVAL_MS = 30000;

void gameLoopTick() {
  bool handled = processTouchForButtons(topMenuButtons, TOPMENU_BUTTON_COUNT);
  if (!handled) {
    handled = processTouchForButtons(bottomMenuButtons, BOTTOMMENU_BUTTON_COUNT);
  }

  if (!handled) {
    switch (currentGameView) {
      case VIEW_EAT_MENU:
        handled = processTouchForButtons(feedMenuButtons, FEED_MENU_BUTTON_COUNT);
        break;
      case VIEW_PLAY_MENU:
        handled = processTouchForButtons(playMenuButtons, PLAY_MENU_BUTTON_COUNT);
        break;
      case VIEW_TOILET_MENU:
        handled = processTouchForButtons(toiletMenuButtons, TOILET_MENU_BUTTON_COUNT);
        break;
      default:
        break;
    }
  }

  unsigned long now = millis();
  unsigned long elapsed = now - lastGameTickMillis;
  if (elapsed >= GAME_TICK_INTERVAL_MS) {
    float dtSeconds = static_cast<float>(elapsed) / 1000.0f;
    lastGameTickMillis = now;
    advanceGameClock(dtSeconds);
    updateNeeds(dtSeconds);
    if (petLifeStageJustChanged()) {
      setLastAction(getLifeStageChangeMessage(getLastLifeStageForEvents(), currentPet.lifeStage), false);
      applyLifeStageChangeEffects(currentPet.lifeStage);
      drawGameScreenDynamic();
      clearLifeStageChangedFlag();
    }
  }

  if (now - lastAutoActionMillis >= AUTO_ACTION_INTERVAL_MS) {
    chooseAndApplyAutoAction();
    lastAutoActionMillis = now;
  }

  if (now - lastAutoSaveMillis >= AUTO_SAVE_INTERVAL_MS) {
    petSaveToStorage();
    lastAutoSaveMillis = now;
  }

  if (now - lastRedrawMillis >= GAME_REDRAW_INTERVAL_MS) {
    drawGameScreenDynamic();
    lastRedrawMillis = now;
  }
}
