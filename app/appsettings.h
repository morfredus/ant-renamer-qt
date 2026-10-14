#pragma once
// Program options (the Options dialog), persisted in an INI file. Defaults are the Delphi ones.
//
// Storage: portable when "antrenamer-qt.ini" sits next to the program (everything stays in that folder),
// otherwise in the user's profile (%APPDATA%\AntRenamerQt\Ant Renamer Qt\antrenamer-qt.ini).
#include <QSettings>
#include <QString>

QSettings appSettings();
bool isPortable();
// Moves the settings between the program folder and the user profile. False if the target is not writable.
bool setPortable(bool on);

struct AppOptions {
    // Display
    bool realtime = true;        // update the list while renaming
    bool resizeCols = false;     // files list columns share the width
    bool fileIcons = true;
    bool forceFont = false;      // use `fontName`/`fontSize` for the whole program
    QString fontName;
    int fontSize = 9;
    int iconSet = 0;             // 0 Scrows, 1 Windows XP, 2 Windows XP Large
    int historyMax = 20;
    bool autoComplete = false;
    // Files & folders
    bool dragNoAsk = false;
    int pageLayout = 0;          // modern layout: 0 tabs, 1 single screen in bands, 2 single screen in columns
    bool newUi = false;          // modern layout (taken into account at the next start)
    bool naturalSort = true;     // files list sort: natural (2 < 10) instead of alphabetical
    bool launchFile = false;     // double click opens the file
    bool addFiles = true, addFolders = false, recursive = true;  // last "add folders" choices
    // Processing
    bool forceDir = true, detectAbsDir = true, folderExt = false, copy = false, genLog = true;
    bool renameSidecar = true;   // a renamed picture takes its sidecars (.xmp, .dop) along
    bool dstRelativeToFile = true;
    // Events: tab index (0 files, 1 actions, 2 log) or -1 for "do nothing"
    int startSwitch = -1, finishSwitch = -1;
    bool startClearLog = true, finishClearFiles = false, finishClearBatch = false;
    bool saveLog = false, saveLogAppend = true;
    QString saveLogFile;

    void load();
    void save() const;
    void applyToCore() const;  // pushes the processing flags to ren::options()
    void applyFont() const;
};

AppOptions& appOptions();
