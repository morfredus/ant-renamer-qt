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

// --- storage -------------------------------------------------------------------------------------------
static QString portableFile() { return QDir(QCoreApplication::applicationDirPath()).filePath(u"antrenamer-qt.ini"_s); }

static QString profileFile() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(u"antrenamer-qt.ini"_s);
}

bool isPortable() { return QFileInfo::exists(portableFile()); }

QSettings appSettings() {
    const QString path = isPortable() ? portableFile() : profileFile();
    QDir().mkpath(QFileInfo(path).absolutePath());
    return QSettings(path, QSettings::IniFormat);
}

bool setPortable(bool on) {
    if (on == isPortable()) return true;
    const QString from = on ? profileFile() : portableFile();
    const QString to = on ? portableFile() : profileFile();
    {
        QSettings src = appSettings();
        src.sync();
        QDir().mkpath(QFileInfo(to).absolutePath());
        QSettings dst(to, QSettings::IniFormat);
        for (const QString& k : src.allKeys()) dst.setValue(k, src.value(k));
        dst.sync();
        if (dst.status() != QSettings::NoError || !QFileInfo::exists(to)) return false;
    }
    QFile::remove(from);
    return true;
}

// --- options ----------------------------------------------------------------------------------------------
#define KEYS(X)                                                                                  \
    X(realtime) X(resizeCols) X(fileIcons) X(forceFont) X(fontName) X(fontSize) X(iconSet)       \
    X(historyMax) X(autoComplete) X(dragNoAsk) X(launchFile) X(naturalSort) X(newUi) X(pageLayout)                                   \
    X(addFiles) X(addFolders) X(recursive) X(forceDir) X(detectAbsDir) X(folderExt) X(renameSidecar) X(copy)      \
    X(genLog) X(dstRelativeToFile) X(startSwitch) X(finishSwitch) X(startClearLog)               \
    X(finishClearFiles) X(finishClearBatch) X(saveLog) X(saveLogAppend) X(saveLogFile)

void AppOptions::load() {
    QSettings s = appSettings();
#define LOAD(k) k = s.value(QStringLiteral("options/" #k), k).value<decltype(k)>();
    KEYS(LOAD)
#undef LOAD
    // Settings written before the "Modern" set existed carry iconSet=0 although nobody chose it (every key is saved):
    // they take the new default once. From the first save of this version on the choice is the user's own.
    if (!s.contains(QStringLiteral("options/iconSetChosen"))) iconSet = 3;
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
    o.copy = copy;
    o.dstRelativeToFile = dstRelativeToFile;
}

void AppOptions::applyFont() const {
    static const QFont original = QApplication::font();
    if (forceFont && !fontName.isEmpty()) QApplication::setFont(QFont(fontName, fontSize));
    else QApplication::setFont(original);
}
