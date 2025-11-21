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
  STATE_LOAD_PET
};

AppState appState = STATE_TITLE;

// --- Définition de boutons simples ---
struct Button {
  int16_t x, y, w, h;
  const char* label;
};

Button btnNewPet  = { 60, 110, 200, 40, "Créer un nouveau Cydagotchi" };
Button btnLoadPet = { 60, 160, 200, 40, "Load Pet" };

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
void drawButton(const Button& b, uint16_t colorFill, uint16_t colorText) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, colorFill);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, TFT_WHITE);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(colorText, colorFill);
  tft.setTextFont(2);
  tft.drawString(b.label, b.x + b.w / 2, b.y + b.h / 2);
}

void drawMenuScreen() {
  tft.fillScreen(TFT_NAVY);

  // Titre en haut
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_NAVY);
  tft.setTextFont(4);
  tft.drawString("Main Menu", SCREEN_W / 2, 40);

  // Boutons
  drawButton(btnNewPet,  TFT_DARKGREEN, TFT_WHITE);
  drawButton(btnLoadPet, TFT_DARKGREY,  TFT_WHITE);
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


void setup() {
  Serial.begin(115200);

  // --- TACTILE : démarrer le bus SPI dédié + init du contrôleur ---
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(1);      // ou 3 si l’axe Y est inversé

  // --- TFT ---
  tft.init();
  tft.setRotation(1);

  drawTitleScreen();
}

void loop() {
  static uint32_t lastTouchRelease = 0;
  int16_t x, y;

  switch (appState) {
    case STATE_TITLE:
      // On quitte l’écran de titre sur premier tap
      if (getTouch(x, y)) {
        // petit debounce
        delay(150);
        while (ts.touched()) {} // attendre relâchement
        appState = STATE_MENU;
        drawMenuScreen();
      }
      break;

    case STATE_MENU:
      if (getTouch(x, y)) {
        delay(150);
        while (ts.touched()) {}

        if (isInside(btnNewPet, x, y)) {
          Serial.println("New Pet selected");
          appState = STATE_NEW_PET;
          tft.fillScreen(TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.setTextFont(2);
          tft.drawString("Creating a new pet...", SCREEN_W/2, SCREEN_H/2);
          // TODO: ici on enchainera sur la creation de pet
        }
        else if (isInside(btnLoadPet, x, y)) {
          Serial.println("Load Pet selected");
          appState = STATE_LOAD_PET;
          tft.fillScreen(TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.setTextColor(TFT_YELLOW, TFT_BLACK);
          tft.setTextFont(2);
          tft.drawString("Loading pet...", SCREEN_W/2, SCREEN_H/2);
          // TODO: plus tard : chargement SPIFFS
        }
      }
      break;

    case STATE_NEW_PET:
      // pour l'instant on ne fait rien, on restera sur cet écran
      // plus tard : générer gènes + passer à l'écran "jeu"
      break;

    case STATE_LOAD_PET:
      // idem : plus tard -> lecture SPIFFS + écran "jeu"
      break;
  }
}
