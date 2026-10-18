#pragma once
#include <QPalette>

// Colour theme of the program: 0 = follow the system, 1 = light, 2 = dark. Light and dark use the Fusion style with
// their own palette, so they work the same on every Qt 6 version and on every system (an AppImage, for instance, has no
// platform theme to tell it that the desktop is dark).
void applyTheme(int mode, bool fusionWhenSystem);
bool isDarkPalette(const QPalette& p);
