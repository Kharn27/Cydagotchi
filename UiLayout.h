#pragma once

#include <TFT_eSPI.h>
#include <cstddef>
#include "PetModel.h"

// Shared screen layout constants
constexpr int16_t SCREEN_W = 320;
constexpr int16_t SCREEN_H = 240;
constexpr int16_t TOP_MENU_HEIGHT = 32;
constexpr int16_t TOP_CLOCK_AREA_W = 64;
constexpr int16_t BOTTOM_MENU_HEIGHT = TOP_MENU_HEIGHT;  // Harmonized height for top/bottom bars
constexpr int16_t ALERT_AREA_W = 64;
constexpr int16_t ALERT_AREA_H = BOTTOM_MENU_HEIGHT;
constexpr int16_t ALERT_AREA_X = SCREEN_W - ALERT_AREA_W;
constexpr int16_t ALERT_AREA_Y = SCREEN_H - BOTTOM_MENU_HEIGHT;
constexpr uint16_t HUD_BAND_COLOR = TFT_DARKGREY;
constexpr uint16_t HUD_TEXT_COLOR = TFT_WHITE;
constexpr uint16_t HUD_BORDER_COLOR = TFT_WHITE;

constexpr int16_t ACTION_AREA_HEIGHT = 28;
constexpr int16_t ACTION_AREA_Y = SCREEN_H - BOTTOM_MENU_HEIGHT - ACTION_AREA_HEIGHT;

// App state enums are shared so that scene changes can be invoked from helpers.
enum AppState {
  STATE_TITLE,
  STATE_MENU,
  STATE_NEW_PET,
  STATE_GAME
};

// Game view modes shared between actions and UI rendering.
// Game view modes shared between actions and UI rendering.
enum GameView {
  VIEW_MAIN,
  VIEW_STATS,
  VIEW_EAT_MENU,
  VIEW_PLAY_MENU,
  VIEW_WORLD_MENU,
  VIEW_TOILET_MENU,
  VIEW_DUEL_MENU
};

enum TopMenuId {
  TOPMENU_NONE = -1,
  TOPMENU_STATS = 0,
  TOPMENU_EAT,
  TOPMENU_PLAY,
  TOPMENU_WORLD,
  TOPMENU_COUNT
};

enum BottomMenuId {
  BOTTOMMENU_NONE = -1,
  BOTTOMMENU_LIGHTS = 0,
  BOTTOMMENU_TOILET,
  BOTTOMMENU_DUEL,
  BOTTOMMENU_COUNT
};

// Enriched button description reused across screens.
typedef void (*ButtonAction)();

struct Button {
  int16_t x, y, w, h;
  const char* label;
  uint16_t fillColor;
  uint16_t textColor;
  uint16_t borderColor;
  ButtonAction onTap;
  int id = -1;
};

// Global display instance comes from Cydagotchi.ino
extern TFT_eSPI tft;

// Button counts defined in Cydagotchi.ino (extern to allow cross-file access)
extern const size_t MENU_BUTTON_COUNT;
extern const size_t NEWPET_BUTTON_COUNT;
extern const size_t TOPMENU_BUTTON_COUNT;
extern const size_t BOTTOMMENU_BUTTON_COUNT;
extern const size_t FEED_MENU_BUTTON_COUNT;
extern const size_t PLAY_MENU_BUTTON_COUNT;
extern const size_t TOILET_MENU_BUTTON_COUNT;

void drawButton(const Button& b);
void drawTopMenuButton(const Button& b, bool active, bool showCloseIndicator);
void drawNeedRow(const char* label, float value, int16_t x, int16_t y);
void drawGaugeRow(const char* label, float value, int16_t x, int16_t y);
void drawPetFace();

