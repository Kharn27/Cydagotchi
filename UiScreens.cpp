#include <Arduino.h>
#include <cstring>
#include <cmath>
#include "UiScreens.h"
#include "Actions.h"
#include "GameState.h"

extern Button menuButtons[];
extern Button newPetButtons[];
extern Button topMenuButtons[];
extern Button bottomMenuButtons[];
extern const size_t MENU_BUTTON_COUNT;
extern const size_t NEWPET_BUTTON_COUNT;
extern const size_t TOPMENU_BUTTON_COUNT;
extern const size_t BOTTOMMENU_BUTTON_COUNT;

extern PersonalityType newPetPersonality;
extern bool hasNewPetPersonality;
extern char newPetName[16];
extern bool hasNewPetName;

namespace {
void clearGameContentArea() {
  const int16_t contentY = TOP_MENU_HEIGHT;
  const int16_t contentH = ACTION_AREA_Y - TOP_MENU_HEIGHT;
  tft.fillRect(0, contentY, SCREEN_W, contentH, TFT_BLACK);
}

TopMenuId activeTopMenuForView(GameView view) {
  switch (view) {
    case VIEW_STATS: return TOPMENU_STATS;
    case VIEW_EAT_MENU: return TOPMENU_EAT;
    case VIEW_PLAY_MENU: return TOPMENU_PLAY;
    case VIEW_WORLD_MENU: return TOPMENU_WORLD;
    default: return TOPMENU_NONE;
  }
}

BottomMenuId activeBottomMenuForView(GameView view) {
  switch (view) {
    case VIEW_TOILET_MENU: return BOTTOMMENU_TOILET;
    case VIEW_DUEL_MENU: return BOTTOMMENU_DUEL;
    default: return BOTTOMMENU_NONE;
  }
}

bool viewHasCloseIndicator(GameView view) {
  return view != VIEW_MAIN;
}

void drawMenuButton(const Button& b, bool active, bool showCloseIndicator) {
  // Shared drawing routine so top and bottom nav bars use the same visual language.
  drawTopMenuButton(b, active, showCloseIndicator);
}

void drawSimpleWipView(const char* title, const char* subtitle) {
  const int16_t contentY = TOP_MENU_HEIGHT;
  const int16_t contentH = ACTION_AREA_Y - TOP_MENU_HEIGHT;
  tft.fillRect(0, contentY, SCREEN_W, contentH, TFT_BLACK);

  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(title, SCREEN_W / 2, contentY + contentH / 2 - 12);
  tft.setTextFont(2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(subtitle, SCREEN_W / 2, contentY + contentH / 2 + 12);
}
}  // namespace

void drawTopMenuBar() {
  tft.fillRect(0, 0, SCREEN_W, TOP_MENU_HEIGHT, HUD_BAND_COLOR);

  TopMenuId activeId = activeTopMenuForView(currentGameView);
  bool showCloseIndicator = viewHasCloseIndicator(currentGameView);

  for (size_t i = 0; i < TOPMENU_BUTTON_COUNT; ++i) {
    bool active = topMenuButtons[i].id == activeId;
    drawMenuButton(topMenuButtons[i], active, active && showCloseIndicator);
  }
}

void drawBottomMenuBar() {
  tft.fillRect(0, ALERT_AREA_Y, SCREEN_W, BOTTOM_MENU_HEIGHT, HUD_BAND_COLOR);

  BottomMenuId activeId = activeBottomMenuForView(currentGameView);
  bool showCloseIndicator = viewHasCloseIndicator(currentGameView);

  for (size_t i = 0; i < BOTTOMMENU_BUTTON_COUNT; ++i) {
    bool active = bottomMenuButtons[i].id == activeId;
    drawMenuButton(bottomMenuButtons[i], active, active && showCloseIndicator);
  }
}

void drawTitleScreen() {
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.drawString("CYDAGOTCHI", SCREEN_W / 2, SCREEN_H / 2 - 40);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextFont(2);
  tft.drawString("Tap to start", SCREEN_W / 2, SCREEN_H / 2 + 10);
}

void drawMenuScreen() {
  tft.fillScreen(TFT_DARKGREY);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
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

  drawTopMenuBar();

  // Bottom bar mirrors the top bar styling for consistent navigation.
  drawBottomMenuBar();

  tft.fillRect(ALERT_AREA_X, ALERT_AREA_Y, ALERT_AREA_W, ALERT_AREA_H, HUD_BAND_COLOR);
}

void drawGameViewMain(bool headerDirty, bool faceDirty, bool forceClear) {
  const int16_t headerY = TOP_MENU_HEIGHT + 4;
  const int16_t contentH = ACTION_AREA_Y - headerY;

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);

  bool clearAll = forceClear || headerDirty;
  if (clearAll) {
    tft.fillRect(0, headerY, SCREEN_W, contentH, TFT_BLACK);
    headerDirty = true;
    faceDirty = true;
  }

  if (headerDirty) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(String("Nom: ") + currentPet.name, 10, headerY + 6);
    tft.drawString(String("Age: ") + String(currentPet.age, 1) + " j", 10, headerY + 20);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.drawString(String("Caractere: ") + PERSONALITY_MODIFIERS[currentPet.personality].label, 10, headerY + 32);
    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    tft.drawString(String("Stade: ") + getLifeStageLabel(currentPet.lifeStage), 10, headerY + 46);
  }

  if (faceDirty) {
    tft.fillRect(SCREEN_W - 120, headerY, 100, 80, TFT_BLACK);
    drawPetFace();
  }
}

void drawGameViewStats(bool statsDirty) {
  if (!statsDirty) return;

  const int16_t contentY = TOP_MENU_HEIGHT;
  const int16_t contentH = ACTION_AREA_Y - TOP_MENU_HEIGHT;
  tft.fillRect(0, contentY, SCREEN_W, contentH, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);

  const int16_t headerY = contentY + 4;
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(String("Nom: ") + currentPet.name, 10, headerY + 6);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString(String("Caractere: ") + PERSONALITY_MODIFIERS[currentPet.personality].label, 10, headerY + 20);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  tft.drawString(String("Stade: ") + getLifeStageLabel(currentPet.lifeStage), 10, headerY + 34);

  const char* hints[] = {
    "Indice: ton pet a faim.",
    "Indice: ton pet a besoin de repos.",
    "Indice: ton pet veut jouer.",
    "Indice: ton pet a besoin d une toilette.",
    "Indice: ton pet veut explorer."
  };

  float needs[] = { currentPet.hunger, currentPet.energy, currentPet.social, currentPet.cleanliness, currentPet.curiosity };
  size_t minIndex = 0;
  float minNeed = needs[0];
  for (size_t i = 1; i < 5; ++i) {
    if (needs[i] < minNeed) {
      minNeed = needs[i];
      minIndex = i;
    }
  }

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(hints[minIndex], 10, headerY + 48);

  int16_t tableY = headerY + 64;
  int16_t available = (ACTION_AREA_Y - 8) - tableY;
  const int rows = 6;
  int16_t rowHeight = available / rows;

  drawGaugeRow("Mood",   currentPet.mood,         10, tableY + rowHeight * 0);
  drawGaugeRow("Hunger", currentPet.hunger,       10, tableY + rowHeight * 1);
  drawGaugeRow("Energy", currentPet.energy,       10, tableY + rowHeight * 2);
  drawGaugeRow("Social", currentPet.social,       10, tableY + rowHeight * 3);
  drawGaugeRow("Clean",  currentPet.cleanliness,  10, tableY + rowHeight * 4);
  drawGaugeRow("Curio",  currentPet.curiosity,    10, tableY + rowHeight * 5);
}

void drawGameViewEatMenu(bool dirty) {
  if (!dirty) return;
  drawSimpleWipView("Manger (WIP)", "Choisis un repas bientot");
}

void drawGameViewPlayMenu(bool dirty) {
  if (!dirty) return;
  drawSimpleWipView("Menu Jeu (WIP)", "Mini-jeux a venir");
}

void drawGameViewWorldMenu(bool dirty) {
  if (!dirty) return;
  drawSimpleWipView("Menu Monde (WIP)", "Aventure en preparation");
}

void drawGameViewToiletMenu(bool dirty) {
  if (!dirty) return;
  drawSimpleWipView("Menu Toilette (WIP)", "Un petit nettoyage arrive");
}

void drawGameViewDuelMenu(bool dirty) {
  if (!dirty) return;
  drawSimpleWipView("Menu Duel (WIP)", "Mode versus en cours");
}

void drawGameClock(bool force) {
  static int lastHour = -1;
  static int lastMinute = -1;

  int hours = 0;
  int minutes = 0;
  getGameTime(hours, minutes);

  if (!force && hours == lastHour && minutes == lastMinute) {
    return;
  }

  lastHour = hours;
  lastMinute = minutes;

  const int16_t clockW = 64;
  const int16_t clockH = 18;
  const int16_t x = SCREEN_W - clockW - 6;
  const int16_t y = TOP_MENU_HEIGHT + 90;

  tft.fillRect(x, y, clockW, clockH, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02d:%02d", hours, minutes);
  tft.drawString(buffer, x + clockW / 2, y + clockH / 2);
}

void drawGameScreenDynamic() {
  static Pet cachedPet = {};
  static bool drawInitialized = false;
  static char cachedAction[sizeof(lastActionText)] = "";
  static bool cachedActionAuto = false;
  static GameView cachedView = VIEW_MAIN;

  auto valueChanged = [](float a, float b, float epsilon) {
    return fabsf(a - b) >= epsilon;
  };

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);

  bool viewChanged = !drawInitialized || cachedView != currentGameView;

  bool headerDirty = !drawInitialized || viewChanged ||
                     strncmp(cachedPet.name, currentPet.name, sizeof(currentPet.name)) != 0 ||
                     cachedPet.personality != currentPet.personality || cachedPet.lifeStage != currentPet.lifeStage ||
                     valueChanged(cachedPet.age, currentPet.age, 0.05f);
  bool needsDirty = !drawInitialized || viewChanged || valueChanged(cachedPet.mood, currentPet.mood, 0.01f) ||
                    valueChanged(cachedPet.hunger, currentPet.hunger, 0.01f) ||
                    valueChanged(cachedPet.energy, currentPet.energy, 0.01f) ||
                    valueChanged(cachedPet.social, currentPet.social, 0.01f) ||
                    valueChanged(cachedPet.cleanliness, currentPet.cleanliness, 0.01f) ||
                    valueChanged(cachedPet.curiosity, currentPet.curiosity, 0.01f);
  bool faceDirty = !drawInitialized || viewChanged || valueChanged(cachedPet.mood, currentPet.mood, 0.02f);
  bool actionDirty = !drawInitialized || viewChanged || cachedActionAuto != lastActionIsAuto ||
                     strncmp(cachedAction, lastActionText, sizeof(lastActionText)) != 0;

  bool alertDirty = !drawInitialized || needsDirty || viewChanged;

  if (viewChanged) {
    // When switching views, rebuild the top bar, clear the content zone, and force a full redraw
    // so the first frame of the Stats panel is properly aligned (no delayed resync).
    drawTopMenuBar();
    drawBottomMenuBar();
    clearGameContentArea();
    headerDirty = true;
    needsDirty = true;
    faceDirty = true;
    actionDirty = true;
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);
  }

  if (currentGameView == VIEW_MAIN) {
    drawGameViewMain(headerDirty, faceDirty, viewChanged);
  } else if (currentGameView == VIEW_STATS) {
    bool statsDirty = headerDirty || needsDirty || faceDirty || viewChanged;
    drawGameViewStats(statsDirty);
  } else if (currentGameView == VIEW_EAT_MENU) {
    bool feedDirty = viewChanged;
    drawGameViewEatMenu(feedDirty);
  } else if (currentGameView == VIEW_PLAY_MENU) {
    bool playDirty = viewChanged;
    drawGameViewPlayMenu(playDirty);
  } else if (currentGameView == VIEW_WORLD_MENU) {
    bool worldDirty = viewChanged;
    drawGameViewWorldMenu(worldDirty);
  } else if (currentGameView == VIEW_TOILET_MENU) {
    bool toiletDirty = viewChanged;
    drawGameViewToiletMenu(toiletDirty);
  } else if (currentGameView == VIEW_DUEL_MENU) {
    bool duelDirty = viewChanged;
    drawGameViewDuelMenu(duelDirty);
  }

  drawGameClock(viewChanged);

  if (actionDirty || viewChanged) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(2);
    uint16_t color = lastActionIsAuto ? TFT_CYAN : TFT_ORANGE;
    tft.setTextColor(color, TFT_BLACK);
    tft.fillRect(0, ACTION_AREA_Y, SCREEN_W, ACTION_AREA_HEIGHT, TFT_BLACK);
    tft.drawString(lastActionText, 10, ACTION_AREA_Y + 6);
  }

  if (alertDirty) {
    drawAlertIcon();
  }

  cachedPet = currentPet;
  drawInitialized = true;
  cachedView = currentGameView;
  strncpy(cachedAction, lastActionText, sizeof(cachedAction));
  cachedAction[sizeof(cachedAction) - 1] = '\0';
  cachedActionAuto = lastActionIsAuto;
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

