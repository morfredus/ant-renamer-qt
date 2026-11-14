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
#include <QSaveFile>
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

#ifdef AR_HAVE_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#endif
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
    // A failure (file gone, access denied) gives INVALID_FILE_ATTRIBUTES, all bits set: it is not a system file.
    const DWORD attr = GetFileAttributesW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(fi.absoluteFilePath()).utf16()));
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_SYSTEM) != 0;
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

// Portal formats offered by file managers on a drag: in a Flatpak without --filesystem the host paths of text/uri-list are
// invisible, the key must be exchanged for sandbox-readable paths through the document portal (FileTransfer).
inline bool hasDroppablePaths(const QMimeData* m) {
    return m->hasUrls() || m->hasFormat(u"application/vnd.portal.filetransfer"_s) || m->hasFormat(u"application/vnd.portal.files"_s);
}

inline QStringList droppedPaths(const QMimeData* m) {
    QStringList paths;
#ifdef AR_HAVE_DBUS
    for (const QString& fmt : {u"application/vnd.portal.filetransfer"_s, u"application/vnd.portal.files"_s}) {
        if (!m->hasFormat(fmt)) {
            continue;
        }
        const QString key = QString::fromUtf8(m->data(fmt)).remove(QChar(0)).trimmed();
        QDBusMessage call = QDBusMessage::createMethodCall(u"org.freedesktop.portal.Documents"_s,
                                                           u"/org/freedesktop/portal/documents"_s,
                                                           u"org.freedesktop.portal.FileTransfer"_s, u"RetrieveFiles"_s);
        call << key << QVariantMap();
        const QDBusMessage reply = QDBusConnection::sessionBus().call(call, QDBus::Block, 5000);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            paths = reply.arguments().at(0).toStringList();
        }
        if (!paths.isEmpty()) {
            for (QString& p : paths) {
                p = QDir::toNativeSeparators(p);
            }
            return paths;
        }
    }
#endif
    for (const QUrl& u : m->urls()) {
        if (u.isLocalFile()) {
            paths << QDir::toNativeSeparators(u.toLocalFile());
        }
    }
    return paths;
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
