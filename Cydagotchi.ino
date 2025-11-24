// Bibliothèques principales
#include <SPI.h>
#include <cstring>
#include <cmath>
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
const int16_t TOP_MENU_HEIGHT = 32;
const int16_t BOTTOM_MENU_HEIGHT = 40;
const int16_t ALERT_AREA_W = 64;
const int16_t ALERT_AREA_H = BOTTOM_MENU_HEIGHT;
const int16_t ALERT_AREA_X = SCREEN_W - ALERT_AREA_W;
const int16_t ALERT_AREA_Y = SCREEN_H - BOTTOM_MENU_HEIGHT;
const uint16_t HUD_BAND_COLOR = TFT_DARKGREY;
const uint16_t HUD_TEXT_COLOR = TFT_WHITE;
const uint16_t HUD_BORDER_COLOR = TFT_WHITE;

const int16_t ACTION_AREA_HEIGHT = 28;
const int16_t ACTION_AREA_Y = SCREEN_H - BOTTOM_MENU_HEIGHT - ACTION_AREA_HEIGHT;

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
bool lightsOff = false;

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
void actionToggleLights();
void actionDuel();
void actionShowStats();
void actionStubWorld();
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
  { 70, 210, 180, 30, "Demarrer le jeu", TFT_GREEN, TFT_WHITE, TFT_WHITE, actionStartGameFromNewPet }
};
Button topMenuButtons[] = {
  { 0, 0, SCREEN_W / 4, TOP_MENU_HEIGHT, "Stats", TFT_DARKCYAN, TFT_WHITE, HUD_BORDER_COLOR, actionShowStats },
  { SCREEN_W / 4, 0, SCREEN_W / 4, TOP_MENU_HEIGHT, "Manger", TFT_DARKGREEN, TFT_WHITE, HUD_BORDER_COLOR, actionEat },
  { (SCREEN_W / 4) * 2, 0, SCREEN_W / 4, TOP_MENU_HEIGHT, "Jeu", TFT_BLUE, TFT_WHITE, HUD_BORDER_COLOR, actionPlay },
  { (SCREEN_W / 4) * 3, 0, SCREEN_W / 4, TOP_MENU_HEIGHT, "Monde", TFT_MAGENTA, TFT_WHITE, HUD_BORDER_COLOR, actionStubWorld }
};

const int16_t bottomButtonWidth = (SCREEN_W - ALERT_AREA_W) / 3;
Button bottomMenuButtons[] = {
  { 0, ALERT_AREA_Y, bottomButtonWidth, BOTTOM_MENU_HEIGHT, "Lumiere", TFT_DARKGREY, TFT_WHITE, HUD_BORDER_COLOR, actionToggleLights },
  { bottomButtonWidth, ALERT_AREA_Y, bottomButtonWidth, BOTTOM_MENU_HEIGHT, "Toilette", TFT_CYAN, TFT_BLACK, HUD_BORDER_COLOR, actionWash },
  { bottomButtonWidth * 2, ALERT_AREA_Y, bottomButtonWidth, BOTTOM_MENU_HEIGHT, "Duel", TFT_RED, TFT_WHITE, HUD_BORDER_COLOR, actionDuel }
};

const size_t MENU_BUTTON_COUNT      = sizeof(menuButtons) / sizeof(menuButtons[0]);
const size_t NEWPET_BUTTON_COUNT    = sizeof(newPetButtons) / sizeof(newPetButtons[0]);
const size_t TOPMENU_BUTTON_COUNT   = sizeof(topMenuButtons) / sizeof(topMenuButtons[0]);
const size_t BOTTOMMENU_BUTTON_COUNT = sizeof(bottomMenuButtons) / sizeof(bottomMenuButtons[0]);

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

const char* getLifeStageChangeMessage(LifeStage previousStage, LifeStage newStage) {
  if (previousStage == STAGE_BABY && newStage == STAGE_TEEN) {
    return "Cydy quitte l'enfance !";
  }
  if (previousStage == STAGE_TEEN && newStage == STAGE_ADULT) {
    return "Cydy atteint l'age adulte";
  }
  if (previousStage == STAGE_ADULT && newStage == STAGE_SENIOR) {
    return "Cydy devient un vieux sage";
  }

  switch (newStage) {
    case STAGE_BABY: return "Cydy vient de naitre !";
    case STAGE_TEEN: return "Cydy devient ado !";
    case STAGE_ADULT: return "Cydy est maintenant adulte !";
    case STAGE_SENIOR: return "Cydy prend de l'age";
    default: return "Cydy evolue !";
  }
}

void applyLifeStageChangeEffects(LifeStage newStage) {
  switch (newStage) {
    case STAGE_TEEN:
      addNeed(currentPet.curiosity, 0.15f);
      addNeed(currentPet.social, 0.12f);
      addNeed(currentPet.cleanliness, -0.05f);
      break;
    case STAGE_ADULT:
      addNeed(currentPet.energy, 0.10f);
      addNeed(currentPet.hunger, 0.08f);
      break;
    case STAGE_SENIOR:
      addNeed(currentPet.social, 0.10f);
      addNeed(currentPet.energy, -0.12f);
      break;
    default:
      break;
  }

  updateMood();
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
void drawGameScreenStatic();
void drawGameScreenDynamic();
void drawAlertIcon();
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
        char birthMsg[32];
        snprintf(birthMsg, sizeof(birthMsg), "%s vient de naitre", currentPet.name);
        setLastAction(birthMsg, false);
      }
      lastGameTickMillis = millis();
      lastAutoActionMillis = lastGameTickMillis;
      lastRedrawMillis = lastGameTickMillis;
      lastAutoSaveMillis = lastGameTickMillis;
      drawGameScreenStatic();
      drawGameScreenDynamic();
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

  if (actionToFire) {
    // Debounce + attendre relâchement
    delay(150);
    while (ts.touched()) {}
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

void drawGameScreenStatic() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);

  // Bandeau haut
  tft.fillRect(0, 0, SCREEN_W, TOP_MENU_HEIGHT, HUD_BAND_COLOR);
  for (size_t i = 0; i < TOPMENU_BUTTON_COUNT; ++i) {
    drawButton(topMenuButtons[i]);
  }

  // Bandeau bas
  tft.fillRect(0, ALERT_AREA_Y, SCREEN_W, BOTTOM_MENU_HEIGHT, HUD_BAND_COLOR);
  for (size_t i = 0; i < BOTTOMMENU_BUTTON_COUNT; ++i) {
    drawButton(bottomMenuButtons[i]);
  }

  // Zone alerte réservée
  tft.fillRect(ALERT_AREA_X, ALERT_AREA_Y, ALERT_AREA_W, ALERT_AREA_H, HUD_BAND_COLOR);
}

void drawGameScreenDynamic() {
  static Pet cachedPet = {};
  static bool drawInitialized = false;
  static char cachedAction[sizeof(lastActionText)] = "";
  static bool cachedActionAuto = false;

  auto valueChanged = [](float a, float b, float epsilon) {
    return fabsf(a - b) >= epsilon;
  };

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);

  bool headerDirty = !drawInitialized || strncmp(cachedPet.name, currentPet.name, sizeof(currentPet.name)) != 0 ||
                     cachedPet.personality != currentPet.personality || cachedPet.lifeStage != currentPet.lifeStage ||
                     valueChanged(cachedPet.age, currentPet.age, 0.05f);
  bool needsDirty = !drawInitialized || valueChanged(cachedPet.mood, currentPet.mood, 0.01f) ||
                    valueChanged(cachedPet.hunger, currentPet.hunger, 0.01f) ||
                    valueChanged(cachedPet.energy, currentPet.energy, 0.01f) ||
                    valueChanged(cachedPet.social, currentPet.social, 0.01f) ||
                    valueChanged(cachedPet.cleanliness, currentPet.cleanliness, 0.01f) ||
                    valueChanged(cachedPet.curiosity, currentPet.curiosity, 0.01f);
  bool faceDirty = !drawInitialized || valueChanged(cachedPet.mood, currentPet.mood, 0.02f);
  bool actionDirty = !drawInitialized || cachedActionAuto != lastActionIsAuto ||
                     strncmp(cachedAction, lastActionText, sizeof(lastActionText)) != 0;

  bool alertDirty = !drawInitialized || needsDirty;
  const int16_t headerY = TOP_MENU_HEIGHT + 4;
  const int16_t headerH = 52;
  const int16_t needsY = headerY + headerH + 4;
  const int16_t needsH = ACTION_AREA_Y - needsY - 4;

  if (headerDirty) {
    tft.fillRect(0, headerY, 210, headerH, TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(String("Nom: ") + currentPet.name, 10, headerY + 6);
    tft.drawString(String("Age: ") + String(currentPet.age, 1) + " j", 10, headerY + 20);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.drawString(String("Caractere: ") + PERSONALITY_MODIFIERS[currentPet.personality].label, 10, headerY + 32);
    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    tft.drawString(String("Stade: ") + getLifeStageLabel(currentPet.lifeStage), 10, headerY + 46);
  }

  if (needsDirty) {
    tft.fillRect(0, needsY, 210, needsH, TFT_BLACK);
    uint16_t moodColor = currentPet.mood >= 0.7f ? TFT_GREEN : (currentPet.mood >= 0.4f ? TFT_YELLOW : TFT_RED);
    tft.setTextColor(moodColor, TFT_BLACK);
    drawNeedRow("Humeur", currentPet.mood, 10, needsY + 6);

    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    drawNeedRow("Hunger", currentPet.hunger, 10, needsY + 18);
    drawNeedRow("Energy", currentPet.energy, 10, needsY + 30);
    drawNeedRow("Social", currentPet.social, 10, needsY + 42);
    drawNeedRow("Clean", currentPet.cleanliness, 10, needsY + 54);
    drawNeedRow("Curio", currentPet.curiosity, 10, needsY + 62);
  }

  if (faceDirty) {
    tft.fillRect(SCREEN_W - 120, headerY, 100, 80, TFT_BLACK);
    drawPetFace();
  }

  if (actionDirty) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);
    uint16_t color = lastActionIsAuto ? TFT_CYAN : TFT_ORANGE;
    tft.setTextColor(color, TFT_BLACK);
    tft.fillRect(0, ACTION_AREA_Y, SCREEN_W, ACTION_AREA_HEIGHT, TFT_BLACK);
    // Placé juste au-dessus de la rangée de boutons bas
    tft.drawString(lastActionText, 10, ACTION_AREA_Y + 6);
    strncpy(cachedAction, lastActionText, sizeof(cachedAction));
    cachedAction[sizeof(cachedAction) - 1] = '\0';
    cachedActionAuto = lastActionIsAuto;
  }

  if (alertDirty) {
    drawAlertIcon();
  }

  cachedPet = currentPet;
  drawInitialized = true;
}

void drawAlertIcon() {
  float needs[] = { currentPet.hunger, currentPet.energy, currentPet.social, currentPet.cleanliness, currentPet.curiosity };
  enum AlertType { ALERT_HUNGER = 0, ALERT_ENERGY, ALERT_SOCIAL, ALERT_CLEAN, ALERT_CURIO };
  AlertType alertType = ALERT_HUNGER;

  float minNeed = needs[0];
  for (size_t i = 1; i < 5; ++i) {
    if (needs[i] < minNeed) {
      minNeed = needs[i];
      alertType = static_cast<AlertType>(i);
    }
  }

  tft.fillRect(ALERT_AREA_X, ALERT_AREA_Y, ALERT_AREA_W, ALERT_AREA_H, HUD_BAND_COLOR);

  if (minNeed > 0.6f) {
    return;
  }

  int16_t cx = ALERT_AREA_X + ALERT_AREA_W / 2;
  int16_t cy = ALERT_AREA_Y + ALERT_AREA_H / 2;

  switch (alertType) {
    case ALERT_HUNGER:
      tft.fillCircle(cx - 6, cy, 9, TFT_ORANGE);
      tft.fillRect(cx + 2, cy - 3, 10, 6, TFT_ORANGE);
      tft.fillCircle(cx + 12, cy - 3, 3, TFT_WHITE);
      break;
    case ALERT_ENERGY:
      tft.setTextDatum(MC_DATUM);
      tft.setTextFont(2);
      tft.setTextColor(TFT_YELLOW, HUD_BAND_COLOR);
      tft.drawString("Zz", cx, cy);
      break;
    case ALERT_SOCIAL:
      tft.fillCircle(cx - 10, cy, 6, TFT_CYAN);
      tft.fillCircle(cx + 6, cy, 6, TFT_CYAN);
      tft.fillCircle(cx - 14, cy - 6, 2, TFT_WHITE);
      tft.fillCircle(cx + 2, cy - 6, 2, TFT_WHITE);
      break;
    case ALERT_CLEAN:
      tft.fillCircle(cx - 6, cy - 4, 3, TFT_BLUE);
      tft.fillCircle(cx + 2, cy + 2, 4, TFT_BLUE);
      tft.fillCircle(cx + 10, cy - 3, 2, TFT_BLUE);
      tft.setTextDatum(TL_DATUM);
      tft.setTextColor(TFT_WHITE, HUD_BAND_COLOR);
      tft.drawString("~", cx + 14, cy - 8);
      break;
    case ALERT_CURIO:
      tft.setTextDatum(MC_DATUM);
      tft.setTextFont(2);
      tft.setTextColor(TFT_WHITE, HUD_BAND_COLOR);
      tft.drawString("?", cx, cy);
      break;
  }
}

void drawGameScreen() {
  drawGameScreenStatic();
  drawGameScreenDynamic();
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
      if (!processTouchForButtons(topMenuButtons, TOPMENU_BUTTON_COUNT)) {
        processTouchForButtons(bottomMenuButtons, BOTTOMMENU_BUTTON_COUNT);
      }
      {
        unsigned long now = millis();
        unsigned long elapsed = now - lastGameTickMillis;
        if (elapsed >= GAME_TICK_INTERVAL_MS) {
          float dtSeconds = static_cast<float>(elapsed) / 1000.0f;
          lastGameTickMillis = now;
          updateNeeds(dtSeconds);
          if (petLifeStageJustChanged()) {
            setLastAction(getLifeStageChangeMessage(getLastLifeStageForEvents(), currentPet.lifeStage), false);
            applyLifeStageChangeEffects(currentPet.lifeStage);
            drawGameScreenDynamic();
            clearLifeStageChangedFlag();
          }
          // On met juste à jour la logique, sans redessiner à chaque tick
        }
    
        // Les auto-actions, elles, redessinent l'écran
        if (now - lastAutoActionMillis >= AUTO_ACTION_INTERVAL_MS) {
          chooseAndApplyAutoAction();   // cette fonction appelle déjà drawGameScreenDynamic()
          lastAutoActionMillis = now;
        }

        if (now - lastAutoSaveMillis >= AUTO_SAVE_INTERVAL_MS) {
          petSaveToStorage();
          lastAutoSaveMillis = now;
        }

        if (now - lastRedrawMillis >= GAME_REDRAW_INTERVAL_MS) {
          drawGameScreenDynamic();
          lastRedrawMillis = now;
        }
      }
      break;
  }
}

void actionEat() {
  petApplyEat();
  lastAutoActionMillis = millis();             // reset du timer auto
  setLastAction("Tu lui donnes a manger", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionSleep() {
  petApplySleep();
  lastAutoActionMillis = millis();
  setLastAction("Tu le mets au dodo", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionPlay() {
  petApplyPlay();
  lastAutoActionMillis = millis();
  setLastAction("Tu joues avec lui", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionWash() {
  petApplyWash();
  lastAutoActionMillis = millis();
  setLastAction("Tu le laves", false);
  petSaveToStorage();
  drawGameScreenDynamic();
}

void actionToggleLights() {
  lightsOff = !lightsOff;
  tft.invertDisplay(lightsOff);
  setLastAction(lightsOff ? "Lumiere OFF" : "Lumiere ON", false);
  drawGameScreenDynamic();
}

void actionDuel() {
  setLastAction("Duel (WIP)", false);
  drawGameScreenDynamic();
}

void actionShowStats() {
  setLastAction("Affichage des stats (WIP)", false);
  drawGameScreenDynamic();
}

void actionStubWorld() {
  setLastAction("Monde / arene (WIP)", false);
  drawGameScreenDynamic();
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
  drawGameScreenDynamic();
}
