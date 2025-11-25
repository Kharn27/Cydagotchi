#pragma once

#include "UiLayout.h"
#include "PetLogic.h"
#include "PetStorage.h"

// Shared state for UI/action feedback
extern char lastActionText[32];
extern bool lastActionIsAuto;
extern bool lightsOff;

// Selection state for NEW_PET screen
extern PersonalityType newPetPersonality;
extern bool hasNewPetPersonality;
extern const char* PRESET_NAMES[];
extern const size_t PRESET_NAME_COUNT;
extern char newPetName[16];
extern uint8_t newPetNameIndex;
extern bool hasNewPetName;

void setLastAction(const char* text, bool isAuto);
const char* getLifeStageChangeMessage(LifeStage previousStage, LifeStage newStage);
void applyLifeStageChangeEffects(LifeStage newStage);

void actionStartNewPet();
void actionLoadPet();
void actionStartGameFromNewPet();
void actionEat();
void actionSleep();
void actionPlay();
void actionWash();
void actionToggleLights();
void actionShowPlayMenu();
void actionShowStats();
void actionShowFeed();
void actionShowWorldMenu();
void actionShowToiletMenu();
void actionShowDuelMenu();
void actionPrevPersonality();
void actionNextPersonality();
void actionPrevName();
void actionNextName();

void chooseAndApplyAutoAction();

