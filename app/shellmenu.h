#pragma once
// Windows Explorer context menu: an "Add to Ant Renamer Qt" entry on files and folders.
//
// Registered for the current user under HKCU\Software\Classes\{*,Directory}\shell\AntRenamerQt, the classic
// menu (Windows 11: "Show more options" or Shift+F10). Explorer starts the program once per selected
// item with `--shell "<path>"`; main() forwards those calls to the instance that is already running.
#include <QString>

bool shellMenuInstalled();
bool setShellMenu(bool on, const QString& label);  // false if the registry could not be written (or not Windows)
