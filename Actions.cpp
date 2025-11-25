#include <Arduino.h>
#include <cstring>
#include "Actions.h"
#include "UiScreens.h"
#include "GameState.h"

extern Pet currentPet;
extern bool petInitialized;

void changeScene(AppState next);

// NEW_PET selections and last action tracking are defined here to keep the main file lighter.
PersonalityType newPetPersonality = PERSO_EQUILIBRE;
bool hasNewPetPersonality = false;
const char* PRESET_NAMES[] = { "Cydy", "Bytey", "Pixy", "Nibbles", "Nova", "Gizmo" };
const size_t PRESET_NAME_COUNT = sizeof(PRESET_NAMES) / sizeof(PRESET_NAMES[0]);

char newPetName[16] = "Cydy";
uint8_t newPetNameIndex = 0;
bool hasNewPetName = false;

char lastActionText[32] = "Bienvenue !";
bool lastActionIsAuto = false;
bool lightsOff = false;

// Helper to leave secondary views (like stats) when performing gameplay actions.
static void ensureMainGameView() {
  if (currentGameView != VIEW_MAIN) {
    currentGameView = VIEW_MAIN;
    drawGameScreenStatic();
    drawGameScreenDynamic();
  }
}

static void toggleMenuView(GameView targetView, const char* openText, const char* closeText) {
  bool entering = currentGameView != targetView;
  currentGameView = entering ? targetView : VIEW_MAIN;
  if (openText && closeText) {
    setLastAction(entering ? openText : closeText, false);
  }
  drawGameScreenDynamic();
}

static void copyCurrentPresetName() {
  strncpy(newPetName, PRESET_NAMES[newPetNameIndex], sizeof(newPetName));
  newPetName[sizeof(newPetName) - 1] = '\0';
}

void setLastAction(const char* text, bool isAuto) {
  strncpy(lastActionText, text, sizeof(lastActionText));
  lastActionText[sizeof(lastActionText) - 1] = '\0';
  lastActionIsAuto = isAuto;
}

const char* getLifeStageChangeMessage(LifeStage previousStage, LifeStage newStage) {
  if (previousStage == STAGE_BABY && newStage == STAGE_TEEN) {
    return "Cydy quitte l'enfance !";
  }
  if (previousStage == STAGE_TEEN && newStage == STAGE_ADULT) {
    return "Cydy atteint l'age adulte";
  }
  if (previousStage == STAGE_ADULT && newStage == STAGE_SENIOR) {
    return "Cydy devient un vieux sage";
  }

  switch (newStage) {
    case STAGE_BABY: return "Cydy vient de naitre !";
    case STAGE_TEEN: return "Cydy devient ado !";
    case STAGE_ADULT: return "Cydy est maintenant adulte !";
    case STAGE_SENIOR: return "Cydy prend de l'age";
    default: return "Cydy evolue !";
  }
}

void applyLifeStageChangeEffects(LifeStage newStage) {
  switch (newStage) {
    case STAGE_TEEN:
      addNeed(currentPet.curiosity, 0.15f);
      addNeed(currentPet.social, 0.12f);
      addNeed(currentPet.cleanliness, -0.05f);
      break;
    case STAGE_ADULT:
      addNeed(currentPet.energy, 0.10f);
      addNeed(currentPet.hunger, 0.08f);
      break;
    case STAGE_SENIOR:
      addNeed(currentPet.social, 0.10f);
      addNeed(currentPet.energy, -0.12f);
      break;
    default:
      break;
  }

  updateMood();
}

void actionStartNewPet() {
  Serial.println("New Pet selected");
  changeScene(STATE_NEW_PET);
}

void actionLoadPet() {
  Serial.println("Load Pet selected");
  hasNewPetPersonality = false;
  hasNewPetName = false;
  bool loaded = petLoadFromStorage();
  if (!loaded) {
    petInitialized = false;
    setLastAction("Pas de sauvegarde, nouveau pet", false);
  } else {
    setLastAction("Pet charge depuis la sauvegarde", false);
  }
  changeScene(STATE_GAME);
}

void actionStartGameFromNewPet() {
  Serial.println("Starting game from NEW_PET");
  petInitialized = false;
  changeScene(STATE_GAME);
}

void actionPrevPersonality() {
  if (newPetPersonality == 0) {
    newPetPersonality = static_cast<PersonalityType>(PERSO_COUNT - 1);
  } else {
    newPetPersonality = static_cast<PersonalityType>(newPetPersonality - 1);
  }
  drawNewPetScreen();
}

void actionNextPersonality() {
  newPetPersonality = static_cast<PersonalityType>((newPetPersonality + 1) % PERSO_COUNT);
  drawNewPetScreen();
}

void actionPrevName() {
  if (newPetNameIndex == 0) {
    newPetNameIndex = PRESET_NAME_COUNT - 1;
  } else {
    newPetNameIndex--;
  }
  copyCurrentPresetName();
  drawNewPetScreen();
}

void actionNextName() {
  newPetNameIndex = (newPetNameIndex + 1) % PRESET_NAME_COUNT;
  copyCurrentPresetName();
  drawNewPetScreen();
}

void actionEat() {
  ensureMainGameView();
  petApplyEat();
  lastAutoActionMillis = millis();
  setLastAction("Tu lui donnes a manger", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionSleep() {
  ensureMainGameView();
  petApplySleep();
  lastAutoActionMillis = millis();
  setLastAction("Tu le mets au dodo", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionPlay() {
  ensureMainGameView();
  petApplyPlay();
  lastAutoActionMillis = millis();
  setLastAction("Tu joues avec lui", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionWash() {
  ensureMainGameView();
  petApplyWash();
  lastAutoActionMillis = millis();
  setLastAction("Tu le laves", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionToggleLights() {
  ensureMainGameView();
  lightsOff = !lightsOff;
  tft.invertDisplay(lightsOff);
  setLastAction(lightsOff ? "Lumiere OFF" : "Lumiere ON", false);
  drawGameScreenDynamic();
}

void actionShowStats() {
  toggleMenuView(VIEW_STATS, "Affichage des stats", "Retour a la vue principale");
}

void actionShowFeed() {
  toggleMenuView(VIEW_EAT_MENU, "Menu Manger (WIP)", "Retour a la vue principale");
}

void actionShowPlayMenu() {
  toggleMenuView(VIEW_PLAY_MENU, "Menu Jeu (WIP)", "Retour a la vue principale");
}

void actionShowWorldMenu() {
  toggleMenuView(VIEW_WORLD_MENU, "Menu Monde (WIP)", "Retour a la vue principale");
}

void actionShowToiletMenu() {
  toggleMenuView(VIEW_TOILET_MENU, "Menu Toilette (WIP)", "Retour a la vue principale");
}

void actionShowDuelMenu() {
  toggleMenuView(VIEW_DUEL_MENU, "Menu Duel (WIP)", "Retour a la vue principale");
}

void chooseAndApplyAutoAction() {
  PetActionType act = petChooseAutoAction();

  switch (act) {
    case PET_ACT_EAT:
      petApplyEat();
      setLastAction("Cydy va manger tout seul", true);
      break;
    case PET_ACT_SLEEP:
      petApplySleep();
      setLastAction("Cydy va dormir tout seul", true);
      break;
    case PET_ACT_PLAY:
      petApplyPlay();
      setLastAction("Cydy joue tout seul", true);
      break;
    case PET_ACT_WASH:
      petApplyWash();
      setLastAction("Cydy se lave tout seul", true);
      break;
  }

  drawGameScreenDynamic();
}

