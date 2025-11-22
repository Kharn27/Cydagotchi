// Bibliothèques principales
#include <SPI.h>
#include <cstring>
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
  char name[16];
  float age;
  float hunger;
  float energy;
  float social;
  float cleanliness;
  float curiosity;
  float mood;
};

Pet currentPet;
bool petInitialized = false;
unsigned long lastGameTickMillis = 0;

const unsigned long GAME_TICK_INTERVAL_MS = 400;  // 0.4 s par tick

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

// Actions applicatives
void applyPlayerActionEat();
void applyPlayerActionSleep();
void applyPlayerActionPlay();
void applyPlayerActionWash();
void updateMood();

// --- Définition des boutons par scène ---
Button menuButtons[] = {
  { 60, 110, 200, 40, "Nouveau Cydagotchi", TFT_DARKGREEN, TFT_WHITE, TFT_WHITE, actionStartNewPet },
  { 60, 160, 200, 40, "Load Pet", TFT_DARKGREY, TFT_WHITE, TFT_WHITE, actionLoadPet }
};

Button newPetButtons[] = {
  { 70, 170, 180, 40, "Démarrer le jeu", TFT_GREEN, TFT_WHITE, TFT_WHITE, actionStartGameFromNewPet }
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

// --- Helpers Pet ---
float clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

void updateMood() {
  float sum = currentPet.hunger + currentPet.energy + currentPet.social + currentPet.cleanliness + currentPet.curiosity;
  currentPet.mood = clamp01(sum / 5.0f);
}

// Mise à jour passive des besoins (appelée en jeu)
void updateNeeds(float dtSeconds) {
  currentPet.hunger = clamp01(currentPet.hunger - 0.01f * dtSeconds);
  currentPet.energy = clamp01(currentPet.energy - 0.008f * dtSeconds);
  currentPet.social = clamp01(currentPet.social - 0.005f * dtSeconds);
  currentPet.cleanliness = clamp01(currentPet.cleanliness - 0.004f * dtSeconds);
  currentPet.curiosity = clamp01(currentPet.curiosity + 0.003f * dtSeconds);

  updateMood();
}

void initDefaultPet() {
  strncpy(currentPet.name, "Cydy", sizeof(currentPet.name));
  currentPet.name[sizeof(currentPet.name) - 1] = '\0';
  currentPet.age = 0.0f;
  currentPet.hunger = 0.5f;
  currentPet.energy = 0.5f;
  currentPet.social = 0.5f;
  currentPet.cleanliness = 0.5f;
  currentPet.curiosity = 0.5f;
  updateMood();
  petInitialized = true;
}

void addNeed(float &need, float delta) {
  need = clamp01(need + delta);
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
      if (!petInitialized) initDefaultPet();
      lastGameTickMillis = millis();
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
  tft.drawString("Nom par defaut : Cydy", SCREEN_W / 2, 100);
  
  for (size_t i = 0; i < NEWPET_BUTTON_COUNT; ++i) {
    drawButton(newPetButtons[i]);
  }
}

void drawGameScreen() {
  if (!petInitialized) {
    initDefaultPet();
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(String("Nom: ") + currentPet.name, 10, 10);
  tft.drawString(String("Age: ") + String(currentPet.age, 1), 10, 28);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  drawNeedRow("Humeur", currentPet.mood, 10, 46);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  drawNeedRow("Hunger", currentPet.hunger, 10, 66);
  drawNeedRow("Energy", currentPet.energy, 10, 82);
  drawNeedRow("Social", currentPet.social, 10, 98);
  drawNeedRow("Clean", currentPet.cleanliness, 10, 114);
  drawNeedRow("Curio", currentPet.curiosity, 10, 130);

  // Petit visage
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.drawString("^_^", SCREEN_W - 70, 70);

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
          float dtSeconds = elapsed / 1000.0f;
          lastGameTickMillis = now;
          updateNeeds(dtSeconds);
          drawGameScreen();
        }
      }
      break;
  }
}

// --- Actions de gameplay ---
void applyPlayerActionEat() {
  addNeed(currentPet.hunger, 0.35f);
  addNeed(currentPet.energy, 0.05f);
  addNeed(currentPet.cleanliness, -0.05f);
  updateMood();
}

void applyPlayerActionSleep() {
  addNeed(currentPet.energy, 0.35f);
  addNeed(currentPet.hunger, 0.10f);
  updateMood();
}

void applyPlayerActionPlay() {
  addNeed(currentPet.social, 0.25f);
  addNeed(currentPet.curiosity, 0.20f);
  addNeed(currentPet.energy, -0.15f);
  addNeed(currentPet.cleanliness, -0.05f);
  updateMood();
}

void applyPlayerActionWash() {
  addNeed(currentPet.cleanliness, 0.40f);
  addNeed(currentPet.energy, -0.05f);
  updateMood();
}

void actionEat() {
  applyPlayerActionEat();
  drawGameScreen();
}

void actionSleep() {
  applyPlayerActionSleep();
  drawGameScreen();
}

void actionPlay() {
  applyPlayerActionPlay();
  drawGameScreen();
}

void actionWash() {
  applyPlayerActionWash();
  drawGameScreen();
}

// --- Actions de boutons ---
void actionStartNewPet() {
  Serial.println("New Pet selected");
  changeScene(STATE_NEW_PET);
}

void actionLoadPet() {
  Serial.println("Load Pet selected");
  // En attendant le module de chargement, on entre en jeu directement
  changeScene(STATE_GAME);
}

void actionStartGameFromNewPet() {
  Serial.println("Starting game from NEW_PET");
  changeScene(STATE_GAME);
}
