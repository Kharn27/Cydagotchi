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

struct LifeStageModifiers {
  float hungerMul;
  float energyMul;
};

static const LifeStageModifiers LIFE_STAGE_MODIFIERS[STAGE_COUNT] = {
  {1.15f, 1.0f},  // Baby: mange plus souvent
  {1.05f, 1.05f}, // Teen: bouge beaucoup
  {1.0f, 1.0f},   // Adult
  {1.0f, 1.1f}    // Senior: fatigue plus vite
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

LifeStage computeLifeStage(float age) {
  if (age < 2.0f) return STAGE_BABY;
  if (age < 7.0f) return STAGE_TEEN;
  if (age < 14.0f) return STAGE_ADULT;
  return STAGE_SENIOR;
}

void updateLifeStage() {
  currentPet.lifeStage = computeLifeStage(currentPet.age);
}

const char* getLifeStageLabel(LifeStage stage) {
  switch (stage) {
    case STAGE_BABY: return "Bebe";
    case STAGE_TEEN: return "Ado";
    case STAGE_ADULT: return "Adulte";
    case STAGE_SENIOR: return "Vieux";
    default: return "?";
  }
}

void updateNeeds(float dtSeconds) {
  const PersonalityModifiers& mods = getPersonalityModifiers(currentPet);
  const LifeStageModifiers& stageMods = LIFE_STAGE_MODIFIERS[currentPet.lifeStage];

  currentPet.hunger      = clamp01(currentPet.hunger - 0.01f  * mods.hungerMul * stageMods.hungerMul * dtSeconds);
  currentPet.energy      = clamp01(currentPet.energy - 0.008f * mods.energyMul * stageMods.energyMul * dtSeconds);
  currentPet.social      = clamp01(currentPet.social - 0.005f * mods.socialMul * dtSeconds);
  currentPet.cleanliness = clamp01(currentPet.cleanliness - 0.004f * mods.cleanMul  * dtSeconds);
  currentPet.curiosity   = clamp01(currentPet.curiosity + 0.003f * mods.curioMul * dtSeconds);
  currentPet.age        += dtSeconds / 60.0f;  // 1 minute réelle = 1 jour virtuel

  updateLifeStage();
  updateMood();
}

void initPetWithPersonality(PersonalityType p, const char* name) {
  const char* effectiveName = (name && name[0] != '\0') ? name : "Cydy";

  strncpy(currentPet.name, effectiveName, sizeof(currentPet.name));
  currentPet.name[sizeof(currentPet.name) - 1] = '\0';

  currentPet.age = 0.0f;
  currentPet.hunger      = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.energy      = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.social      = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.cleanliness = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));
  currentPet.curiosity   = clamp01(0.5f + (static_cast<float>(random(-10, 11)) / 100.0f));

  currentPet.personality = p;
  currentPet.lifeStage = STAGE_BABY;
  updateMood();
  petInitialized = true;

  Serial.print("Personnalite : ");
  Serial.println(PERSONALITY_MODIFIERS[currentPet.personality].label);
}

void initDefaultPet() {
  PersonalityType randomPerso = static_cast<PersonalityType>(random(PERSO_COUNT));
  initPetWithPersonality(randomPerso, "Cydy");
}
