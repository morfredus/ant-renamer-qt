#include "devaids.h"

#include "dialogs.h"
#include "helpwindow.h"
#include "icons.h"
#include "mainwindow.h"
#include "ren_actions.h"

#include <QApplication>
#include <QListWidget>
#include <QTabWidget>
#include <QTimer>

namespace {

constexpr int kActionTypes = int(ren::ActionType::Exif) + 1;  // entries of the list on the Actions page

}  // namespace

void runDeveloperAids(MainWindow& w, QApplication& app) {
    // Developer aid: ANTREN_MISSING=1 also builds every dialog so their untranslated strings get logged.
    if (qEnvironmentVariableIsSet("ANTREN_MISSING")) {
        OptionsDialog d1(&w);
        AboutDialog d2(&w);
        DragDropDialog d3(&w);
        AddFoldersDialog d4({}, &w);
    }
    // Developer aid: ANTREN_SHOT=out.png renders the window (optionally switching to ANTREN_TAB) and exits.
    if (const QByteArray shot = qgetenv("ANTREN_SHOT"); !shot.isEmpty()) {
        if (const QByteArray tab = qgetenv("ANTREN_TAB"); !tab.isEmpty()) {
            w.showTab(tab.toInt());
        }
        // ANTREN_ACTION=n picks action n (0-13) on the Actions page; ANTREN_PREVIEW=1 refreshes the preview (F5);
        // ANTREN_SIZE=WxH sizes the window.
        if (const QByteArray act = qgetenv("ANTREN_ACTION"); !act.isEmpty()) {
            for (auto* l : w.findChildren<QListWidget*>()) {
                if (l->count() == kActionTypes) {
                    l->setCurrentRow(act.toInt());
                }
            }
        }
        if (qEnvironmentVariableIsSet("ANTREN_PREVIEW")) {  // from the event loop: the preview waits for its worker thread
            QTimer::singleShot(0, &w, [&w] {
                for (auto* a : w.findChildren<QAction*>()) {
                    if (a->shortcut() == QKeySequence(Qt::Key_F5)) {
                        a->trigger();
                        break;
                    }
                }
            });
        }
        if (const int n = qEnvironmentVariableIntValue("ANTREN_THEME_TOGGLE");
            n > 0) {  // presses the light / dark button n times
            QTimer::singleShot(0, &w, [&w, n] {
                for (auto* a : w.findChildren<QAction*>()) {
                    if (a->data().toInt() == TbTheme && a->data().isValid()) {
                        for (int i = 0; i < n; ++i) {
                            a->trigger();
                        }
                        break;
                    }
                }
            });
        }
        if (const QByteArray size = qgetenv("ANTREN_SIZE"); size.contains('x')) {  // e.g. 900x640
            w.resize(size.split('x')[0].toInt(), size.split('x')[1].toInt());
        }
        QTimer::singleShot(qEnvironmentVariableIntValue("ANTREN_DELAY") + 300, [&w, &app, shot] {
            const QByteArray which = qgetenv("ANTREN_DIALOG");  // "options" / "about": grab that dialog instead
            if (which == "help") {  // the help window: grab the visible top-level that is not the main window
                showHelp(QString::fromLocal8Bit(qgetenv("ANTREN_HELPTOPIC").isEmpty() ? "index" : qgetenv("ANTREN_HELPTOPIC")),
                         &w);
                for (QWidget* t : QApplication::topLevelWidgets()) {
                    if (t != &w && t->isVisible() && t->windowTitle().contains(QStringLiteral(" - "))) {
                        t->grab().save(QString::fromLocal8Bit(shot));
                        break;
                    }
                }
            } else if (which == "options" || which == "about") {
                QDialog* d = which == "options" ? static_cast<QDialog*>(new OptionsDialog(&w))
                                                : static_cast<QDialog*>(new AboutDialog(&w));
                d->show();
                if (const QByteArray tab = qgetenv("ANTREN_DTAB"); !tab.isEmpty()) {
                    if (auto* tw = d->findChild<QTabWidget*>()) {
                        tw->setCurrentIndex(tab.toInt());
                    }
                }
                d->grab().save(QString::fromLocal8Bit(shot));
            } else {
                w.grab().save(QString::fromLocal8Bit(shot));
            }
            app.quit();
        });
    }
}
