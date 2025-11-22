#include <SPI.h>                       // << AJOUT
#include <TFT_eSPI.h>
//#include <XPT2046_Touchscreen.h>       // << je te conseille la lib officielle
 #include <XPT2046_Touchscreen_TT.h>  // tu peux commenter celle-là

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

// --- Modèle de pet ---
struct Pet {
  String name;
  float ageDays;
  float hunger;
  float energy;
  float social;
  float cleanliness;
  float curiosity;
  float mood;
};

Pet currentPet;
bool petInitialized = false;

void initDefaultPet();
void recalcMood(Pet &pet);
void clampNeed(float &value);

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
void actionPlayerEat();
void actionPlayerSleep();
void actionPlayerPlay();
void actionPlayerWash();

// --- Définition des boutons par scène ---
Button menuButtons[] = {
  { 60, 110, 200, 40, "Nouveau Cydagotchi", TFT_DARKGREEN, TFT_WHITE, TFT_WHITE, actionStartNewPet },
  { 60, 160, 200, 40, "Load Pet", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionLoadPet }
};

Button newPetButtons[] = {
  { 70, 170, 180, 40, "Démarrer le jeu", TFT_GREEN, TFT_WHITE, TFT_WHITE, actionStartGameFromNewPet }
};

Button gameButtons[] = {
  { 15, 185, 70, 40, "Mange", TFT_DARKGREEN, TFT_WHITE, TFT_WHITE, actionPlayerEat },
  { 95, 185, 70, 40, "Dors", TFT_BLUE, TFT_WHITE, TFT_WHITE, actionPlayerSleep },
  { 175, 185, 70, 40, "Joue", TFT_ORANGE, TFT_WHITE, TFT_WHITE, actionPlayerPlay },
  { 255, 185, 70, 40, "Lave", TFT_CYAN, TFT_BLACK, TFT_WHITE, actionPlayerWash }
};

const size_t MENU_BUTTON_COUNT   = sizeof(menuButtons) / sizeof(menuButtons[0]);
const size_t NEWPET_BUTTON_COUNT = sizeof(newPetButtons) / sizeof(newPetButtons[0]);
const size_t GAME_BUTTON_COUNT   = sizeof(gameButtons) / sizeof(gameButtons[0]);

// --- Utilitaires Pet ---
void clampNeed(float &value) {
  if (value < 0.0f) value = 0.0f;
  if (value > 1.0f) value = 1.0f;
}

void recalcMood(Pet &pet) {
  pet.mood = (pet.hunger + pet.energy + pet.social + pet.cleanliness + pet.curiosity) / 5.0f;
  clampNeed(pet.mood);
}

void initDefaultPet() {
  currentPet.name = "Cyd";
  currentPet.ageDays = 0.0f;
  currentPet.hunger = random(40, 61) / 100.0f;
  currentPet.energy = random(40, 61) / 100.0f;
  currentPet.social = random(40, 61) / 100.0f;
  currentPet.cleanliness = random(40, 61) / 100.0f;
  currentPet.curiosity = random(40, 61) / 100.0f;
  recalcMood(currentPet);
  petInitialized = true;
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

// --- Déclarations écrans ---
void drawMenuScreen();
void drawNewPetScreen();
void drawGameScreen();

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
      drawNewPetScreen();
      break;
    case STATE_GAME:
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
  tft.drawString("Nom choisi : " + currentPet.name, SCREEN_W / 2, 90);

  tft.setTextFont(2);
  tft.drawString("Tap sur \"Démarrer\" pour jouer", SCREEN_W / 2, 120);

  for (size_t i = 0; i < NEWPET_BUTTON_COUNT; ++i) {
    drawButton(newPetButtons[i]);
  }
}

void drawGameScreen() {
  tft.fillScreen(TFT_BLACK);

  // En-tête
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(2);
  tft.drawString("Pet : " + currentPet.name, 10, 10);

  // Visage simple
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(6);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("^_^", SCREEN_W / 2, 70);

  // Besoins
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  int16_t statsY = 130;
  tft.drawString("Faim       : " + String(currentPet.hunger, 2), 20, statsY);
  tft.drawString("Énergie    : " + String(currentPet.energy, 2), 20, statsY + 15);
  tft.drawString("Social     : " + String(currentPet.social, 2), 20, statsY + 30);
  tft.drawString("Propreté   : " + String(currentPet.cleanliness, 2), 20, statsY + 45);
  tft.drawString("Curiosité  : " + String(currentPet.curiosity, 2), 20, statsY + 60);
  tft.drawString("Humeur     : " + String(currentPet.mood, 2), 20, statsY + 75);

  // Boutons d'actions
  for (size_t i = 0; i < GAME_BUTTON_COUNT; ++i) {
    drawButton(gameButtons[i]);
  }
}


void setup() {
  Serial.begin(115200);

  // --- TACTILE : démarrer le bus SPI dédié + init du contrôleur ---
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(1);      // ou 3 si l’axe Y est inversé

  // --- TFT ---
  tft.init();
  tft.setRotation(1);

  initDefaultPet();
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
      break;
  }
}

// --- Actions de boutons ---
void applyPlayerActionEat() {
  currentPet.hunger += 0.30f;
  currentPet.cleanliness -= 0.05f;
  clampNeed(currentPet.hunger);
  clampNeed(currentPet.cleanliness);
  recalcMood(currentPet);
}

void applyPlayerActionSleep() {
  currentPet.energy += 0.40f;
  currentPet.hunger -= 0.10f;
  clampNeed(currentPet.energy);
  clampNeed(currentPet.hunger);
  recalcMood(currentPet);
}

void applyPlayerActionPlay() {
  currentPet.social += 0.20f;
  currentPet.curiosity += 0.20f;
  currentPet.energy -= 0.15f;
  clampNeed(currentPet.social);
  clampNeed(currentPet.curiosity);
  clampNeed(currentPet.energy);
  recalcMood(currentPet);
}

void applyPlayerActionWash() {
  currentPet.cleanliness += 0.45f;
  currentPet.energy -= 0.05f;
  clampNeed(currentPet.cleanliness);
  clampNeed(currentPet.energy);
  recalcMood(currentPet);
}

void actionStartNewPet() {
  Serial.println("New Pet selected");
  changeScene(STATE_NEW_PET);
}

void actionLoadPet() {
  Serial.println("Load Pet selected");
  // En attendant le module de chargement, on entre en jeu directement
  if (!petInitialized) {
    initDefaultPet();
  }
  changeScene(STATE_GAME);
}

void actionStartGameFromNewPet() {
  Serial.println("Starting game from NEW_PET");
  initDefaultPet();
  changeScene(STATE_GAME);
}

void actionPlayerEat() {
  applyPlayerActionEat();
  drawGameScreen();
}

void actionPlayerSleep() {
  applyPlayerActionSleep();
  drawGameScreen();
}

void actionPlayerPlay() {
  applyPlayerActionPlay();
  drawGameScreen();
}

void actionPlayerWash() {
  applyPlayerActionWash();
  drawGameScreen();
}
