#include "PetStorage.h"
#include <Preferences.h>
#include <Arduino.h>
#include <cstring>

static Preferences prefs;
static const char* NAMESPACE = "cydagotchi";
static const uint8_t DATA_VERSION = 1;

void petStorageBegin() {
  prefs.begin(NAMESPACE, false); // false = lecture/écriture
}

void petSaveToStorage() {
  prefs.putUChar("version", DATA_VERSION);
  prefs.putString("name", currentPet.name);
  prefs.putFloat("age", currentPet.age);
  prefs.putFloat("hunger", currentPet.hunger);
  prefs.putFloat("energy", currentPet.energy);
  prefs.putFloat("social", currentPet.social);
  prefs.putFloat("clean", currentPet.cleanliness);
  prefs.putFloat("curio", currentPet.curiosity);
  prefs.putFloat("mood", currentPet.mood);
  prefs.putUChar("perso", static_cast<uint8_t>(currentPet.personality));

  Serial.println("[Storage] Pet saved");
}

bool petLoadFromStorage() {
  uint8_t v = prefs.getUChar("version", 0);
  if (v != DATA_VERSION) {
    Serial.println("[Storage] No valid save (version mismatch)");
    return false;
  }

  String name = prefs.getString("name", "");
  if (name.length() == 0) {
    Serial.println("[Storage] No valid save (empty name)");
    return false;
  }

  strncpy(currentPet.name, name.c_str(), sizeof(currentPet.name));
  currentPet.name[sizeof(currentPet.name) - 1] = '\0';

  currentPet.age         = prefs.getFloat("age", 0.0f);
  currentPet.hunger      = prefs.getFloat("hunger", 0.5f);
  currentPet.energy      = prefs.getFloat("energy", 0.5f);
  currentPet.social      = prefs.getFloat("social", 0.5f);
  currentPet.cleanliness = prefs.getFloat("clean", 0.5f);
  currentPet.curiosity   = prefs.getFloat("curio", 0.5f);
  currentPet.mood        = prefs.getFloat("mood", 0.5f);

  uint8_t persoRaw = prefs.getUChar("perso", 0);
  if (persoRaw >= PERSO_COUNT) persoRaw = 0;
  currentPet.personality = static_cast<PersonalityType>(persoRaw);

  petInitialized = true;
  Serial.println("[Storage] Pet loaded from storage");
  return true;
}

void petClearStorage() {
  prefs.clear();
  Serial.println("[Storage] Save cleared");
}
