#pragma once

#include <Arduino.h>

/*
OLED UI layer.

This file owns OLED display behavior:
- OLED initialization
- normal Wi-Fi/TCP status screen
- SHOW_TEXT / SHOW_NUMBER / CLEAR hold behavior
- INVERT ON/OFF feedback line

Most comments were moved from the original main.cpp into the file where that
logic now lives, so the explanation stays close to the code.
*/

bool oledUiInit();

void showOledMessage(const char *line1,
                     const char *line2,
                     const char *line3,
                     const char *line4,
                     const char *line5 = "");

void oledUiUpdateStatus(bool clientConnected);
void oledUiPeriodicUpdate(bool clientConnected);

void oledUiClearAndHold();
void oledUiShowText(const String &message);
void oledUiShowNumber(const String &numberText);
String oledUiToggleInvert(bool clientConnected);
