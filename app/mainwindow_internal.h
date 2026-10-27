#pragma once
// Helpers shared by the mainwindow*.cpp files: MainWindow is split by topic (construction, files, batch, run).
#include "appsettings.h"
#include "icons.h"
#include "lngtranslator.h"
#include "ren_path.h"
#include "theme.h"
#include "updatecheck.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QCollator>
#include <QDate>
#include <QDragEnterEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
using namespace ren;
using namespace Qt::StringLiterals;

#include "mainwindow.h"

namespace mwdetail {

// Windows refuses the focus to a program that is not in the foreground (the file manager, during a drop): a key press of
// ours first makes the system consider us the active program.
inline void bringToFront(QWidget* w) {
    w->raise();
    w->activateWindow();
#ifdef Q_OS_WIN
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    SetForegroundWindow(reinterpret_cast<HWND>(w->winId()));
#endif
}

// Hidden file: a dot name (also on Windows, where .git or .config carry no attribute), or the hidden / system attribute
// on Windows (desktop.ini, Thumbs.db...).
inline bool isHiddenOrSystem(const QFileInfo& fi) {
    if (fi.isHidden() || fi.fileName().startsWith(u'.')) {
        return true;
    }
#ifdef Q_OS_WIN
    return GetFileAttributesW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(fi.absoluteFilePath()).utf16())) &
           FILE_ATTRIBUTE_SYSTEM;
#else
    return false;
#endif
}

inline constexpr int SortRole = Qt::UserRole + 1;
inline constexpr int IdRole =
    Qt::UserRole;  // ren::File::id() / ren::Action::id() of the row: looked up again, never dereferenced

inline QString native(const QString& p) {
    return QDir::toNativeSeparators(p);
}

inline QString tr_(const char* s) {
    return QCoreApplication::translate("Ren", s);
}

// Sorts like the original: text case-insensitively, size and dates numerically.
class FileItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem& o) const override {
        const int c = treeWidget()->sortColumn();
        const QVariant a = data(c, SortRole), b = o.data(c, SortRole);
        if (a.isValid() && b.isValid()) {
            return a.toLongLong() < b.toLongLong();
        }
        static QCollator natural = [] {
            QCollator q;
            q.setNumericMode(true);
            q.setCaseSensitivity(Qt::CaseInsensitive);
            return q;
        }();
        if (appOptions().naturalSort) {
            return natural.compare(text(c), o.text(c)) < 0;
        }
        return text(c).compare(o.text(c), Qt::CaseInsensitive) < 0;
    }
};

inline QColor folderColor(const QPalette& p) {  // window colour darkened by 8 per channel, like the original
    const QColor base = p.color(QPalette::Base);
    return QColor(qMax(0, base.red() - 8), qMax(0, base.green() - 8), qMax(0, base.blue() - 8));
}

}  // namespace mwdetail

using namespace mwdetail;
