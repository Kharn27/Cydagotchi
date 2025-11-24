#include <Arduino.h>
#include "UiLayout.h"

extern Pet currentPet;

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

void drawGaugeRow(const char* label, float value, int16_t x, int16_t y) {
  const int segments = 10;
  char gauge[segments + 3];

  gauge[0] = '[';
  for (int i = 0; i < segments; ++i) {
    gauge[i + 1] = (i < value * segments) ? '#' : '-';
  }
  gauge[segments + 1] = ']';
  gauge[segments + 2] = '\0';

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%s: %s", label, gauge);

  tft.drawString(buffer, x, y);

  char percent[8];
  snprintf(percent, sizeof(percent), "%3d%%", static_cast<int>(value * 100.0f + 0.5f));

  int16_t percentX = x + 70 + (segments + 2) * 6 + 8;
  tft.drawString(percent, percentX, y);
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

