// Bibliothèques principales
#include <SPI.h>
#include <cstring>
#include <TFT_eSPI.h>
//#include <XPT2046_Touchscreen.h>       // << je te conseille la lib officielle
 #include <XPT2046_Touchscreen_TT.h>  // tu peux commenter celle-là

#include "PetModel.h"
#include "PetLogic.h"
#include "PetStorage.h"

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

TFT_eSPI tft = TFT_eSPI();

// SPI séparé pour le tactile
SPIClass touchscreenSPI = SPIClass(VSPI);          // << AJOUT
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);   // inchangé


const int16_t SCREEN_W = 320;
const int16_t SCREEN_H = 240;

// --- États du programme ---
enum AppState {
  STATE_TITLE,
  STATE_MENU,
  STATE_NEW_PET,
  STATE_GAME
};

AppState appState = STATE_TITLE;

// Journal d'action (dernière action effectuée)
char lastActionText[32] = "Bienvenue !";
bool lastActionIsAuto = false;

// Sélection de personnalité pour l'écran NEW_PET
PersonalityType newPetPersonality = PERSO_EQUILIBRE;
bool hasNewPetPersonality = false;

// Sélection de nom pour l'écran NEW_PET
const char* PRESET_NAMES[] = { "Cydy", "Bytey", "Pixy", "Nibbles", "Nova", "Gizmo" };
const size_t PRESET_NAME_COUNT = sizeof(PRESET_NAMES) / sizeof(PRESET_NAMES[0]);

char newPetName[16] = "Cydy";
uint8_t newPetNameIndex = 0;
bool hasNewPetName = false;

// --- Définition de boutons enrichis ---
typedef void (*ButtonAction)();

struct Button {
  int16_t x, y, w, h;
  const char* label;
  uint16_t fillColor;
  uint16_t textColor;
  uint16_t borderColor;
  ButtonAction onTap;
};

// Déclarations de callbacks de boutons
void actionStartNewPet();
void actionLoadPet();
void actionStartGameFromNewPet();
void actionEat();
void actionSleep();
void actionPlay();
void actionWash();
void actionPrevPersonality();
void actionNextPersonality();
void actionPrevName();
void actionNextName();

// --- Définition des boutons par scène ---
Button menuButtons[] = {
  { 60, 110, 200, 40, "Nouveau Cydagotchi", TFT_DARKGREEN, TFT_WHITE, TFT_WHITE, actionStartNewPet },
  { 60, 160, 200, 40, "Load Pet", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionLoadPet }
};

Button newPetButtons[] = {
  // Personnalité
  { 40, 140, 50, 30, "<P",  TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionPrevPersonality },
  { 230, 140, 50, 30, "P>", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionNextPersonality },

  // Nom
  { 40, 175, 50, 30, "<N",  TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionPrevName },
  { 230, 175, 50, 30, "N>", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionNextName },

  // Démarrer le jeu
  { 70, 210, 180, 30, "Démarrer le jeu", TFT_GREEN, TFT_WHITE, TFT_WHITE, actionStartGameFromNewPet }
};

Button gameButtons[] = {
  { 20, 180, 70, 40, "Mange", TFT_DARKGREEN, TFT_WHITE, TFT_WHITE, actionEat },
  { 95, 180, 70, 40, "Dors", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionSleep },
  { 170, 180, 70, 40, "Joue", TFT_BLUE, TFT_WHITE, TFT_WHITE, actionPlay },
  { 245, 180, 70, 40, "Lave", TFT_CYAN, TFT_BLACK, TFT_WHITE, actionWash }
};

const size_t MENU_BUTTON_COUNT   = sizeof(menuButtons) / sizeof(menuButtons[0]);
const size_t NEWPET_BUTTON_COUNT = sizeof(newPetButtons) / sizeof(newPetButtons[0]);
const size_t GAME_BUTTON_COUNT   = sizeof(gameButtons) / sizeof(gameButtons[0]);

const unsigned long GAME_TICK_INTERVAL_MS = 400;
const unsigned long AUTO_ACTION_INTERVAL_MS = 6000;
const unsigned long GAME_REDRAW_INTERVAL_MS = 1000;
const unsigned long AUTO_SAVE_INTERVAL_MS = 30000;
unsigned long lastGameTickMillis = 0;
unsigned long lastAutoActionMillis = 0;
unsigned long lastRedrawMillis = 0;
unsigned long lastAutoSaveMillis = 0;

void setLastAction(const char* text, bool isAuto) {
  strncpy(lastActionText, text, sizeof(lastActionText));
  lastActionText[sizeof(lastActionText) - 1] = '\0';
  lastActionIsAuto = isAuto;
}

// --- Dessins d'écrans ---
void drawTitleScreen() {
  tft.fillScreen(TFT_BLACK);

  // Titre
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.drawString("CYDAGOTCHI", SCREEN_W / 2, SCREEN_H / 2 - 40);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextFont(2);
  tft.drawString("Tap to start", SCREEN_W / 2, SCREEN_H / 2 + 10);
}

// Dessine un bouton rectangulaire avec un texte centré
void drawButton(const Button& b) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, b.fillColor);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, b.borderColor);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(b.textColor, b.fillColor);
  tft.setTextFont(2);
  tft.drawString(b.label, b.x + b.w / 2, b.y + b.h / 2);
}

void drawNeedRow(const char* label, float value, int16_t x, int16_t y) {
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "%s: %.0f%%", label, value * 100.0f);
  tft.drawString(buffer, x, y);
}

void drawPetFace() {
  uint16_t faceColor;
  const char* faceExpression;

  if (currentPet.mood >= 0.7f) {
    faceColor = TFT_GREEN;
    faceExpression = "^_^";
  } else if (currentPet.mood >= 0.4f) {
    faceColor = TFT_YELLOW;
    faceExpression = "-_-";
  } else {
    faceColor = TFT_ORANGE;
    faceExpression = "T_T";
  }

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(faceColor, TFT_BLACK);
  tft.setTextFont(4);
  tft.drawString(faceExpression, SCREEN_W - 70, 70);
}

// --- Déclarations écrans ---
void drawMenuScreen();
void drawNewPetScreen();
void drawGameScreen();
void chooseAndApplyAutoAction();

// --- Gestion centralisée des scènes ---
void changeScene(AppState next) {
  appState = next;

  switch (appState) {
    case STATE_TITLE:
      drawTitleScreen();
      break;
    case STATE_MENU:
      drawMenuScreen();
      break;
    case STATE_NEW_PET:
      if (!hasNewPetPersonality) {
        newPetPersonality = PERSO_EQUILIBRE;
        hasNewPetPersonality = true;
      }

      if (!hasNewPetName) {
        newPetNameIndex = 0;
        strncpy(newPetName, PRESET_NAMES[newPetNameIndex], sizeof(newPetName));
        newPetName[sizeof(newPetName) - 1] = '\0';
        hasNewPetName = true;
      }
      drawNewPetScreen();
      break;
    case STATE_GAME:
      if (!petInitialized) {
        if (hasNewPetPersonality && hasNewPetName) {
          initPetWithPersonality(newPetPersonality, newPetName);
        } else if (hasNewPetPersonality) {
          initPetWithPersonality(newPetPersonality, "Cydy");
        } else {
          initDefaultPet();
        }
        setLastAction("Nouveau pet cree", false);
      }
      lastGameTickMillis = millis();
      lastAutoActionMillis = lastGameTickMillis;
      lastRedrawMillis = lastGameTickMillis;
      lastAutoSaveMillis = lastGameTickMillis;
      drawGameScreen();
      break;
  }
}

bool getTouch(int16_t &x, int16_t &y) {
  if (!ts.tirqTouched() || !ts.touched()) return false;

  TS_Point p = ts.getPoint();

  x = map(p.x, 200, 3700, 0, SCREEN_W);
  y = map(p.y, 240, 3800, 0, SCREEN_H);

  // clamp
  if (x < 0) x = 0;
  if (x >= SCREEN_W) x = SCREEN_W - 1;
  if (y < 0) y = 0;
  if (y >= SCREEN_H) y = SCREEN_H - 1;

  // debug
  Serial.print("Touch x="); Serial.print(x);
  Serial.print(" y="); Serial.print(y);
  Serial.print(" z="); Serial.println(p.z);

  return true;
}


bool isInside(const Button& b, int16_t x, int16_t y) {
  return (x >= b.x && x <= b.x + b.w &&
          y >= b.y && y <= b.y + b.h);
}

// Gestion centralisée du tactile pour une liste de boutons
bool processTouchForButtons(Button* buttons, size_t count) {
  int16_t x, y;
  if (!getTouch(x, y)) return false;

  ButtonAction actionToFire = nullptr;

  for (size_t i = 0; i < count; ++i) {
    if (isInside(buttons[i], x, y)) {
      actionToFire = buttons[i].onTap;
      break;
    }
  }

  // Debounce + attendre relâchement
  delay(150);
  while (ts.touched()) {}

  if (actionToFire) {
    actionToFire();
    return true;
  }

  return false;
}

// --- Dessins d'écrans ---
void drawMenuScreen() {
  tft.fillScreen(TFT_NAVY);

  // Titre en haut
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_NAVY);
  tft.setTextFont(4);
  tft.drawString("Main Menu", SCREEN_W / 2, 40);

  for (size_t i = 0; i < MENU_BUTTON_COUNT; ++i) {
    drawButton(menuButtons[i]);
  }
}

void drawNewPetScreen() {
  tft.fillScreen(TFT_DARKGREY);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextFont(4);
  tft.drawString("Nouveau Pet", SCREEN_W / 2, 40);

  tft.setTextFont(2);
  String nameLine = String("Nom : ") + newPetName;
  tft.drawString(nameLine, SCREEN_W / 2, 100);

  String persoLine = String("Personnalite : ") +
                     PERSONALITY_MODIFIERS[newPetPersonality].label;
  tft.drawString(persoLine, SCREEN_W / 2, 130);

  for (size_t i = 0; i < NEWPET_BUTTON_COUNT; ++i) {
    drawButton(newPetButtons[i]);
  }
}

void drawGameScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(String("Nom: ") + currentPet.name, 10, 10);
  tft.drawString(String("Age: ") + String(currentPet.age, 1) + " j", 10, 28);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString(String("Caractere: ") + PERSONALITY_MODIFIERS[currentPet.personality].label, 10, 44);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.drawString(String("Stade: ") + getLifeStageLabel(currentPet.lifeStage), 10, 60);

  uint16_t moodColor = currentPet.mood >= 0.7f ? TFT_GREEN : (currentPet.mood >= 0.4f ? TFT_YELLOW : TFT_RED);
  tft.setTextColor(moodColor, TFT_BLACK);
  drawNeedRow("Humeur", currentPet.mood, 10, 76);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  drawNeedRow("Hunger", currentPet.hunger, 10, 96);
  drawNeedRow("Energy", currentPet.energy, 10, 112);
  drawNeedRow("Social", currentPet.social, 10, 128);
  drawNeedRow("Clean", currentPet.cleanliness, 10, 144);
  drawNeedRow("Curio", currentPet.curiosity, 10, 160);

  // Petit visage
  drawPetFace();

  // --- Journal d'action (texte de ce qui vient de se passer) ---
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  uint16_t color = lastActionIsAuto ? TFT_CYAN : TFT_ORANGE;
  tft.setTextColor(color, TFT_BLACK);
  // Placé juste au-dessus de la rangée de boutons (y=180)
  tft.drawString(lastActionText, 10, 176);

  // Boutons d'action
  for (size_t i = 0; i < GAME_BUTTON_COUNT; ++i) {
    drawButton(gameButtons[i]);
  }
}


void setup() {
  Serial.begin(115200);

  // Initialisation du stockage
  petStorageBegin();

  // Seed RNG pour varier la personnalité et les stats de départ
  randomSeed(analogRead(34));

  // --- TACTILE : démarrer le bus SPI dédié + init du contrôleur ---
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(1);      // ou 3 si l’axe Y est inversé

  // --- TFT ---
  tft.init();
  tft.setRotation(1);

  // >>> AJOUT TEST INVERSION <<<
  tft.invertDisplay(true);   // ou false selon ce que tu vois

  changeScene(STATE_TITLE);
}

void loop() {
  switch (appState) {
    case STATE_TITLE:
      // On quitte l’écran de titre sur premier tap
      {
        int16_t x, y;
        if (getTouch(x, y)) {
          delay(150);
          while (ts.touched()) {} // attendre relâchement
          changeScene(STATE_MENU);
        }
      }
      break;

    case STATE_MENU:
      processTouchForButtons(menuButtons, MENU_BUTTON_COUNT);
      break;

    case STATE_NEW_PET:
      // pour l'instant on ne fait rien, on restera sur cet écran
      // plus tard : générer gènes + passer à l'écran "jeu"
      processTouchForButtons(newPetButtons, NEWPET_BUTTON_COUNT);
      break;

    case STATE_GAME:
      processTouchForButtons(gameButtons, GAME_BUTTON_COUNT);
      {
        unsigned long now = millis();
        unsigned long elapsed = now - lastGameTickMillis;
        if (elapsed >= GAME_TICK_INTERVAL_MS) {
          float dtSeconds = static_cast<float>(elapsed) / 1000.0f;
          lastGameTickMillis = now;
          updateNeeds(dtSeconds);
          // On met juste à jour la logique, sans redessiner à chaque tick
        }
    
        // Les auto-actions, elles, redessinent l'écran
        if (now - lastAutoActionMillis >= AUTO_ACTION_INTERVAL_MS) {
          chooseAndApplyAutoAction();   // cette fonction appelle déjà drawGameScreen()
          lastAutoActionMillis = now;
        }

        if (now - lastAutoSaveMillis >= AUTO_SAVE_INTERVAL_MS) {
          petSaveToStorage();
          lastAutoSaveMillis = now;
        }

        if (now - lastRedrawMillis >= GAME_REDRAW_INTERVAL_MS) {
          drawGameScreen();
          lastRedrawMillis = now;
        }
      }
      break;
  }
}

void actionEat() {
  petApplyEat();
  lastAutoActionMillis = millis();             // reset du timer auto
  setLastAction("Tu lui donnes à manger", false);
  petSaveToStorage();
  drawGameScreen();
}

void actionSleep() {
  petApplySleep();
  lastAutoActionMillis = millis();
  setLastAction("Tu le mets au dodo", false);
  petSaveToStorage();
  drawGameScreen();
}

void actionPlay() {
  petApplyPlay();
  lastAutoActionMillis = millis();
  setLastAction("Tu joues avec lui", false);
  petSaveToStorage();
  drawGameScreen();
}

void actionWash() {
  petApplyWash();
  lastAutoActionMillis = millis();
  setLastAction("Tu le laves", false);
  petSaveToStorage();
  drawGameScreen();
}

// --- Actions de boutons ---
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
    petInitialized = false; // pour forcer la création en STATE_GAME
    setLastAction("Pas de sauvegarde, nouveau pet", false);
  } else {
    setLastAction("Pet charge depuis la sauvegarde", false);
  }
  // On entre en jeu directement
  changeScene(STATE_GAME);
}

void actionStartGameFromNewPet() {
  Serial.println("Starting game from NEW_PET");

  // Force la création d'un nouveau pet lors de l'entrée en jeu
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

static void copyCurrentPresetName() {
  strncpy(newPetName, PRESET_NAMES[newPetNameIndex], sizeof(newPetName));
  newPetName[sizeof(newPetName) - 1] = '\0';
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

// --- Auto action utility AI ---
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

  // Affichage après action auto
  drawGameScreen();
}
