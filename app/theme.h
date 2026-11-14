#pragma once
#include <QPalette>

// Colour theme: 0 system, 1 light, 2 dark. Light and dark use Fusion with their own palette, so they behave the same on every
// Qt 6 version and system (an AppImage has no platform theme to report a dark desktop).
void applyTheme(int mode, bool fusionWhenSystem);
bool isDarkPalette(const QPalette& p);
