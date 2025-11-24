#ifndef PET_LOGIC_H
#define PET_LOGIC_H

#include <stdint.h>
#include "PetModel.h"

// Type d'action de haut niveau sur le pet
enum PetActionType : uint8_t {
  PET_ACT_EAT = 0,
  PET_ACT_SLEEP,
  PET_ACT_PLAY,
  PET_ACT_WASH
};

// Actions déclenchées par le joueur
void petApplyEat();
void petApplySleep();
void petApplyPlay();
void petApplyWash();

// IA : choisit l'action la plus pertinente en fonction de l'état du pet
PetActionType petChooseAutoAction();

#endif // PET_LOGIC_H
