#pragma once
// Explorer context menu entry "Add to Ant Renamer Qt", registered for the current user under
// HKCU\Software\Classes\{*,Directory}\shell\AntRenamerQt (classic menu on Windows 11). Explorer starts the program once per
// selected item with `--shell "<path>"`; main() forwards them to the running instance.
#include <QString>

bool shellMenuInstalled();
bool setShellMenu(bool on, const QString& label);  // false if the registry could not be written (or not Windows)
