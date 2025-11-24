#ifndef PET_MODEL_H
#define PET_MODEL_H

#include <stdint.h>

// Personality types for the pet
enum PersonalityType : uint8_t {
  PERSO_EQUILIBRE = 0,
  PERSO_GOURMAND,
  PERSO_PARESSEUX,
  PERSO_SOCIABLE,
  PERSO_CURIEUX,
  PERSO_MANIAQUE,
  PERSO_COUNT
};

struct PersonalityModifiers {
  const char* label;
  float hungerMul;
  float energyMul;
  float socialMul;
  float cleanMul;
  float curioMul;
};

enum LifeStage : uint8_t {
  STAGE_BABY = 0,
  STAGE_TEEN,
  STAGE_ADULT,
  STAGE_SENIOR,
  STAGE_COUNT
};

struct Pet {
  char name[16];
  float age;
  float hunger;
  float energy;
  float social;
  float cleanliness;
  float curiosity;
  float mood;
  PersonalityType personality;
  LifeStage lifeStage;
};

extern Pet currentPet;
extern bool petInitialized;
extern const PersonalityModifiers PERSONALITY_MODIFIERS[PERSO_COUNT];

float clamp01(float v);

void updateMood();
void updateNeeds(float dtSeconds);
LifeStage computeLifeStage(float age);
void updateLifeStage();
void initDefaultPet();
void initPetWithPersonality(PersonalityType p, const char* name);
void addNeed(float& need, float delta);
const char* getLifeStageLabel(LifeStage stage);

#endif // PET_MODEL_H
