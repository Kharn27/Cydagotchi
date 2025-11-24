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
};

extern Pet currentPet;
extern bool petInitialized;
extern const PersonalityModifiers PERSONALITY_MODIFIERS[PERSO_COUNT];

float clamp01(float v);

void updateMood();
void updateNeeds(float dtSeconds);
void initDefaultPet();
void initPetWithPersonality(PersonalityType p);
void addNeed(float& need, float delta);

#endif // PET_MODEL_H
