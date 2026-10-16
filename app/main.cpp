#include "dialogs.h"
#include "helpwindow.h"
#include "lngtranslator.h"
#include "mainwindow.h"
#include "shellmenu.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QTabWidget>
#include <QThread>
#include <QTimer>

namespace {

QString serverName() { return QStringLiteral("AntRenamerQt-shell-") + qEnvironmentVariable("USERNAME", QStringLiteral("user")); }

// Hands paths to the instance that owns the lock. Explorer starts one process per selected item, all at
// once, so the first one may still be starting up: keep trying for a while.
bool forwardToRunningInstance(const QStringList& paths, int timeoutMs) {
    QElapsedTimer t;
    t.start();
    do {
        QLocalSocket s;
        s.connectToServer(serverName());
        if (s.waitForConnected(300)) {
            s.write(paths.join(u'\n').toUtf8());
            s.flush();
            s.waitForBytesWritten(2000);
            s.disconnectFromServer();
            return true;
        }
        QThread::msleep(150);
    } while (t.elapsed() < timeoutMs);
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Ant Renamer Qt"));
    app.setOrganizationName(QStringLiteral("AntRenamerQt"));
    app.setApplicationVersion(QStringLiteral(AR_APP_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/renamer.ico")));

    // Explorer context menu: `--shell <path>` adds the path to the window that is already open.
    const QStringList args = QCoreApplication::arguments().mid(1);
    QLockFile lock(QDir::temp().filePath(QStringLiteral("AntRenamerQt-instance.lock")));
    const bool primary = lock.tryLock(0);
    if (const int shellAt = int(args.indexOf(QStringLiteral("--shell"))); shellAt >= 0 && !primary)
        if (forwardToRunningInstance(args.mid(shellAt + 1), 10000)) return 0;

    QFont base = app.font();  // the original uses 8 pt controls
    base.setPointSize(8);
    app.setFont(base);

    auto* lng = new LngTranslator;  // original .lng files; English when none matches
    if (lng->loadLanguage(languageDir(), defaultLanguage())) {
        app.installTranslator(lng);
        setLngTranslator(lng);
    }

    // Scripting helpers (installers, tests): register / remove the Explorer menu entry and leave.
    if (args.contains(QStringLiteral("--install-shell")) || args.contains(QStringLiteral("--remove-shell"))) {
        const bool on = args.contains(QStringLiteral("--install-shell"));
        return setShellMenu(on, QCoreApplication::translate("Ren", "Add to Ant Renamer Qt")) ? 0 : 1;
    }

    MainWindow w;
    QLocalServer server;
    if (primary) {
        QLocalServer::removeServer(serverName());
        server.listen(serverName());
        QObject::connect(&server, &QLocalServer::newConnection, &w, [&server, &w] {
            while (QLocalSocket* s = server.nextPendingConnection()) {
                QObject::connect(s, &QLocalSocket::readyRead, s, [s] { s->setProperty("buf", s->property("buf").toByteArray() + s->readAll()); });
                QObject::connect(s, &QLocalSocket::disconnected, &w, [s, &w] {
                    const QByteArray data = s->property("buf").toByteArray() + s->readAll();
                    s->deleteLater();
                    w.addShellPaths(QString::fromUtf8(data).split(u'\n', Qt::SkipEmptyParts));
                });
            }
        });
    }
    w.show();
    w.processCommandLine(args);
    // Developer aid: ANTREN_MISSING=1 also builds every dialog so their untranslated strings get logged.
    if (qEnvironmentVariableIsSet("ANTREN_MISSING")) {
        OptionsDialog d1(&w);
        AboutDialog d2(&w);
        DragDropDialog d3(&w);
        AddFoldersDialog d4({}, &w);
    }
    // Developer aid: ANTREN_SHOT=out.png renders the window (optionally switching to ANTREN_TAB) and exits.
    if (const QByteArray shot = qgetenv("ANTREN_SHOT"); !shot.isEmpty()) {
        if (const QByteArray tab = qgetenv("ANTREN_TAB"); !tab.isEmpty()) w.showTab(tab.toInt());
        QTimer::singleShot(qEnvironmentVariableIntValue("ANTREN_DELAY") + 300, [&w, &app, shot] {
            const QByteArray which = qgetenv("ANTREN_DIALOG");  // "options" / "about": grab that dialog instead
            if (which == "help") {  // the help window: grab the visible top-level that is not the main window
                showHelp(QString::fromLocal8Bit(qgetenv("ANTREN_HELPTOPIC").isEmpty() ? "index" : qgetenv("ANTREN_HELPTOPIC")), &w);
                for (QWidget* t : QApplication::topLevelWidgets())
                    if (t != &w && t->isVisible() && t->windowTitle().contains(QStringLiteral(" - "))) { t->grab().save(QString::fromLocal8Bit(shot)); break; }
            } else if (which == "options" || which == "about") {
                QDialog* d = which == "options" ? static_cast<QDialog*>(new OptionsDialog(&w)) : static_cast<QDialog*>(new AboutDialog(&w));
                d->show();
                if (const QByteArray tab = qgetenv("ANTREN_DTAB"); !tab.isEmpty())
                    if (auto* tw = d->findChild<QTabWidget*>()) tw->setCurrentIndex(tab.toInt());
                d->grab().save(QString::fromLocal8Bit(shot));
            } else {
                w.grab().save(QString::fromLocal8Bit(shot));
            }
            app.quit();
        });
    }
    return app.exec();
}
