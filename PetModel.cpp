#include <Arduino.h>
#include "PetModel.h"
#include <cstring>

Pet currentPet;
bool petInitialized = false;

const PersonalityModifiers PERSONALITY_MODIFIERS[PERSO_COUNT] = {
  { "Équilibré", 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
  { "Gourmand", 1.4f, 1.0f, 1.0f, 1.0f, 1.0f },
  { "Paresseux", 1.0f, 1.4f, 1.0f, 1.0f, 1.0f },
  { "Sociable", 1.0f, 1.0f, 1.4f, 1.0f, 1.0f },
  { "Curieux", 1.0f, 1.0f, 1.0f, 1.0f, 1.4f },
  { "Maniaque", 1.0f, 1.0f, 1.0f, 1.4f, 1.0f }
};

static const PersonalityModifiers& getPersonalityModifiers(const Pet& pet) {
  return PERSONALITY_MODIFIERS[pet.personality];
}

float clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

void addNeed(float& need, float delta) {
  need = clamp01(need + delta);
}

void updateMood() {
  float sum = currentPet.hunger + currentPet.energy + currentPet.social + currentPet.cleanliness + currentPet.curiosity;
  currentPet.mood = clamp01(sum / 5.0f);
}

void updateNeeds(float dtSeconds) {
  const PersonalityModifiers& mods = getPersonalityModifiers(currentPet);

  currentPet.hunger      = clamp01(currentPet.hunger - 0.01f  * mods.hungerMul * dtSeconds);
  currentPet.energy      = clamp01(currentPet.energy - 0.008f * mods.energyMul * dtSeconds);
  currentPet.social      = clamp01(currentPet.social - 0.005f * mods.socialMul * dtSeconds);
  currentPet.cleanliness = clamp01(currentPet.cleanliness - 0.004f * mods.cleanMul  * dtSeconds);
  currentPet.curiosity   = clamp01(currentPet.curiosity + 0.003f * mods.curioMul * dtSeconds);
  currentPet.age        += dtSeconds / 60.0f;  // 1 minute réelle = 1 jour virtuel

  updateMood();
}

void initPetWithPersonality(PersonalityType p) {
  strncpy(currentPet.name, "Cydy", sizeof(currentPet.name));
  currentPet.name[sizeof(currentPet.name) - 1] = '\0';
  currentPet.age = 0.0f;
  currentPet.hunger = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.energy = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.social = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.cleanliness = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.curiosity = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.personality = p;
  updateMood();
  petInitialized = true;
  Serial.print("Personnalite : ");
  Serial.println(PERSONALITY_MODIFIERS[currentPet.personality].label);
}

void initDefaultPet() {
  PersonalityType randomPerso = static_cast<PersonalityType>(random(PERSO_COUNT));
  initPetWithPersonality(randomPerso);
}
