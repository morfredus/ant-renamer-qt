#pragma once
// Built-in help viewer: the help pages (Markdown, from the original program's HTML pages) with their table of contents on the
// left. No browser and no absolute path: the pages are read from the "help" folder found next to the program
// (Windows zip / installer, build folder) or in <prefix>/share/antrenamer-qt/help (Linux packages).
#include <QString>

class QWidget;

// Opens (or raises) the help window on `topic` ("index", "format", "regexp", ...: page name without language).
void showHelp(const QString& topic, QWidget* parent = nullptr);
