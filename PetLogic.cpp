#include "PetLogic.h"
#include <Arduino.h>

void petApplyEat() {
  addNeed(currentPet.hunger, 0.35f);
  addNeed(currentPet.energy, 0.05f);
  addNeed(currentPet.cleanliness, -0.05f);
  updateMood();
}

void petApplySleep() {
  addNeed(currentPet.energy, 0.35f);
  addNeed(currentPet.hunger, 0.10f);
  updateMood();
}

void petApplyPlay() {
  addNeed(currentPet.social, 0.25f);
  addNeed(currentPet.curiosity, 0.20f);
  addNeed(currentPet.energy, -0.15f);
  addNeed(currentPet.cleanliness, -0.05f);
  updateMood();
}

void petApplyWash() {
  addNeed(currentPet.cleanliness, 0.40f);
  addNeed(currentPet.energy, -0.05f);
  updateMood();
}

PetActionType petChooseAutoAction() {
  // Scores de manque (1 = besoin urgent)
  float hungerNeed = 1.0f - currentPet.hunger;
  float energyNeed = 1.0f - currentPet.energy;
  float socialNeed = 1.0f - currentPet.social;
  float cleanNeed  = 1.0f - currentPet.cleanliness;

  float needScores[] = { hungerNeed, energyNeed, socialNeed, cleanNeed };

  float maxScore = needScores[0];
  for (size_t i = 1; i < 4; ++i) {
    if (needScores[i] > maxScore) {
      maxScore = needScores[i];
    }
  }

  // Tolérance pour permettre un choix aléatoire entre besoins proches
  const float tolerance = 0.05f;
  PetActionType candidates[4];
  size_t candidateCount = 0;
  for (size_t i = 0; i < 4; ++i) {
    if (needScores[i] >= maxScore - tolerance) {
      candidates[candidateCount++] = static_cast<PetActionType>(i);
    }
  }

  size_t chosenIndex = random(candidateCount);
  return candidates[chosenIndex];
}
