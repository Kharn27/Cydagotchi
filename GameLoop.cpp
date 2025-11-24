#include <Arduino.h>
#include "GameLoop.h"
#include "Actions.h"
#include "GameState.h"
#include "Input.h"
#include "PetLogic.h"
#include "UiScreens.h"

extern Button topMenuButtons[];
extern Button bottomMenuButtons[];
extern const size_t TOPMENU_BUTTON_COUNT;
extern const size_t BOTTOMMENU_BUTTON_COUNT;

static const unsigned long GAME_TICK_INTERVAL_MS = 400;
static const unsigned long AUTO_ACTION_INTERVAL_MS = 6000;
static const unsigned long GAME_REDRAW_INTERVAL_MS = 1000;
static const unsigned long AUTO_SAVE_INTERVAL_MS = 30000;

void gameLoopTick() {
  if (!processTouchForButtons(topMenuButtons, TOPMENU_BUTTON_COUNT)) {
    processTouchForButtons(bottomMenuButtons, BOTTOMMENU_BUTTON_COUNT);
  }

  unsigned long now = millis();
  unsigned long elapsed = now - lastGameTickMillis;
  if (elapsed >= GAME_TICK_INTERVAL_MS) {
    float dtSeconds = static_cast<float>(elapsed) / 1000.0f;
    lastGameTickMillis = now;
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
