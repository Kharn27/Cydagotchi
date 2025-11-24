#pragma once

#include <Arduino.h>
#include <XPT2046_Touchscreen_TT.h>
#include "UiLayout.h"

extern XPT2046_Touchscreen ts;

bool getTouch(int16_t &x, int16_t &y);
bool isInside(const Button& b, int16_t x, int16_t y);
bool processTouchForButtons(Button* buttons, size_t count);

