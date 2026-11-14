#include "appsettings.h"

#include "ren_files.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

AppOptions& appOptions() {
    static AppOptions o;
    return o;
}

// --- storage ---
static QString portableFile() {
    return QDir(QCoreApplication::applicationDirPath()).filePath(u"antrenamer-qt.ini"_s);
}

static QString profileFile() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(u"antrenamer-qt.ini"_s);
}

bool isPortable() {
    return QFileInfo::exists(portableFile());
}

QSettings appSettings() {
    const QString path = isPortable() ? portableFile() : profileFile();
    QDir().mkpath(QFileInfo(path).absolutePath());
    return QSettings(path, QSettings::IniFormat);
}

static const QString historyName = u"antrenamer-qt-history.json"_s;

QString historyFile() {
    return QFileInfo(appSettings().fileName()).absolutePath() + u'/' + historyName;
}

bool setPortable(bool on) {
    if (on == isPortable()) {
        return true;
    }
    const QString from = on ? profileFile() : portableFile();
    const QString to = on ? portableFile() : profileFile();
    {
        QSettings src = appSettings();
        src.sync();
        QDir().mkpath(QFileInfo(to).absolutePath());
        QSettings dst(to, QSettings::IniFormat);
        for (const QString& k : src.allKeys()) {
            dst.setValue(k, src.value(k));
        }
        dst.sync();
        if (dst.status() != QSettings::NoError || !QFileInfo::exists(to)) {
            return false;
        }
    }
    const QString histFrom = QFileInfo(from).absolutePath() + u'/' + historyName;
    const QString histTo = QFileInfo(to).absolutePath() + u'/' + historyName;
    if (QFileInfo::exists(histFrom)) {  // the history follows the settings; losing it is not worth failing the switch
        QFile::remove(histTo);
        QFile::rename(histFrom, histTo);
    }
    // The old file is what decides the mode while it exists: if it cannot be removed the switch did not happen.
    return QFile::remove(from) || !QFileInfo::exists(from);
}

// --- options ---
// clang-format off
#define KEYS(X)     X(realtime) X(resizeCols) X(fileIcons) X(forceFont) X(fontName) X(fontSize) X(iconSet) X(historyMax)     X(autoComplete) X(dragNoAsk) X(launchFile) X(addHidden) X(naturalSort) X(undoButton) X(newUi)     X(pageLayout) X(theme) X(checkUpdates) X(checkPrereleases) X(lastUpdateCheck) X(addFiles) X(addFolders)     X(recursive) X(forceDir) X(detectAbsDir) X(folderExt) X(renameSidecar) X(numberDuplicates) X(copy) X(genLog)     X(dstRelativeToFile) X(startSwitch) X(finishSwitch) X(startClearLog) X(finishClearFiles)     X(finishClearBatch) X(saveLog) X(saveLogAppend) X(saveLogFile)
// clang-format on

void AppOptions::load() {
    QSettings s = appSettings();
    // First start (no settings file, or an empty one for portable mode): modern layout, single screen in bands, written now so
    // it stays the choice. Existing settings keep their layout. The installer's two update-check keys do not count as settings.
    QStringList keys = s.allKeys();
    keys.removeAll(u"options/checkUpdates"_s);
    keys.removeAll(u"options/checkPrereleases"_s);
    if (keys.isEmpty()) {
        newUi = true;
        pageLayout = 1;
        s.setValue(QStringLiteral("options/newUi"), newUi);
        s.setValue(QStringLiteral("options/pageLayout"), pageLayout);
        s.sync();
    }
#define LOAD(k) k = s.value(QStringLiteral("options/" #k), k).value<decltype(k)>();
    KEYS(LOAD)
#undef LOAD
    // Settings saved before the "Modern" set existed carry iconSet=0 without anyone choosing it: it takes the new default once.
    // Another explicit choice (Windows XP, XP Large) is kept.
    if (!s.contains(QStringLiteral("options/iconSetChosen")) && iconSet == 0) {
        iconSet = kDefaultIconSet;
    }
    // values read from an INI file are never trusted: an index out of range would reach the interface as is
    iconSet = qBound(0, iconSet, iconSetCount() - 1);
    historyMax = qBound(1, historyMax, 200);
    fontSize = qBound(6, fontSize, 32);
    startSwitch = qBound(-1, startSwitch, 2);
    finishSwitch = qBound(-1, finishSwitch, 2);
    pageLayout = qBound(0, pageLayout, 2);
    applyToCore();
}

void AppOptions::save() const {
    QSettings s = appSettings();
#define SAVE(k) s.setValue(QStringLiteral("options/" #k), k);
    KEYS(SAVE)
#undef SAVE
    s.setValue(QStringLiteral("options/iconSetChosen"), true);
}

void AppOptions::applyToCore() const {
    ren::Options& o = ren::options();
    o.forceDir = forceDir;
    o.detectAbsDir = detectAbsDir;
    o.folderExt = folderExt;
    o.renameSidecar = renameSidecar;
    o.numberDuplicates = numberDuplicates;
    o.copy = copy;
    o.dstRelativeToFile = dstRelativeToFile;
}

void AppOptions::applyFont() const {
    static const QFont original = QApplication::font();
    if (forceFont && !fontName.isEmpty()) {
        QApplication::setFont(QFont(fontName, fontSize));
    } else {
        QApplication::setFont(original);
    }
}
