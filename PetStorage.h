#ifndef PET_STORAGE_H
#define PET_STORAGE_H

#include "PetModel.h"

// Initialise le système de stockage (à appeler dans setup)
void petStorageBegin();

// Sauvegarde l'état courant du pet dans la flash
void petSaveToStorage();

// Tente de charger un pet depuis la flash.
// Retourne true si une sauvegarde valide a été trouvée, false sinon.
bool petLoadFromStorage();

// Optionnel : efface la sauvegarde
void petClearStorage();

#endif // PET_STORAGE_H
