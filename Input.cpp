#include "Input.h"

bool getTouch(int16_t &x, int16_t &y) {
  if (!ts.tirqTouched() || !ts.touched()) return false;

  TS_Point p = ts.getPoint();

  x = map(p.x, 200, 3700, 0, SCREEN_W);
  y = map(p.y, 240, 3800, 0, SCREEN_H);

  if (x < 0) x = 0;
  if (x >= SCREEN_W) x = SCREEN_W - 1;
  if (y < 0) y = 0;
  if (y >= SCREEN_H) y = SCREEN_H - 1;

  Serial.print("Touch x="); Serial.print(x);
  Serial.print(" y="); Serial.print(y);
  Serial.print(" z="); Serial.println(p.z);

  return true;
}

bool isInside(const Button& b, int16_t x, int16_t y) {
  return (x >= b.x && x <= b.x + b.w &&
          y >= b.y && y <= b.y + b.h);
}

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
    delay(150);
    while (ts.touched()) {}
    actionToFire();
    return true;
  }

  return false;
}

