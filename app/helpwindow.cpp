#include "helpwindow.h"

#include "appsettings.h"
#include "lngtranslator.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStyle>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace {

class HelpWindow : public QWidget {
public:
    HelpWindow(const QString& dir, const QString& lang, QWidget* parent) : QWidget(parent, Qt::Window), dir_(dir) {
        setAttribute(Qt::WA_DeleteOnClose, false);
        setWindowTitle(QApplication::translate("Ren", "Help") + u" - "_s + QApplication::applicationName());
        resize(900, 600);

        toc_ = new QTreeWidget;
        toc_->setHeaderHidden(true);
        browser_ = new QTextBrowser;
        browser_->setOpenExternalLinks(true);
        browser_->setSearchPaths({dir_});
        QFile css(QDir(dir_).filePath(u"main.css"_s));
        if (css.open(QIODevice::ReadOnly)) browser_->document()->setDefaultStyleSheet(QString::fromUtf8(css.readAll()));

        auto* back = new QPushButton(style()->standardIcon(QStyle::SP_ArrowBack), QString());
        auto* forward = new QPushButton(style()->standardIcon(QStyle::SP_ArrowForward), QString());
        back->setFlat(true);
        forward->setFlat(true);
        back->setToolTip(QApplication::translate("Ren", "Back"));
        forward->setToolTip(QApplication::translate("Ren", "Forward"));
        connect(back, &QPushButton::clicked, browser_, &QTextBrowser::backward);
        connect(forward, &QPushButton::clicked, browser_, &QTextBrowser::forward);
        connect(browser_, &QTextBrowser::backwardAvailable, back, &QWidget::setEnabled);
        connect(browser_, &QTextBrowser::forwardAvailable, forward, &QWidget::setEnabled);
        back->setEnabled(false);
        forward->setEnabled(false);

        auto* right = new QWidget;
        auto* rl = new QVBoxLayout(right);
        rl->setContentsMargins(0, 0, 0, 0);
        auto* bar = new QHBoxLayout;
        bar->addWidget(back);
        bar->addWidget(forward);
        bar->addStretch();
        rl->addLayout(bar);
        rl->addWidget(browser_, 1);

        auto* split = new QSplitter;
        split->addWidget(toc_);
        split->addWidget(right);
        split->setStretchFactor(1, 1);
        split->setSizes({250, 650});
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(4, 4, 4, 4);
        l->addWidget(split);

        loadToc(lang);
        connect(toc_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* it) {
            const QString page = it->data(0, Qt::UserRole).toString();
            if (!page.isEmpty()) open(page);
        });
        connect(browser_, &QTextBrowser::sourceChanged, this, [this](const QUrl& u) { selectInToc(QFileInfo(u.toLocalFile()).fileName()); });
        QSettings s = appSettings();
        if (s.contains(u"help/geometry"_s)) restoreGeometry(s.value(u"help/geometry"_s).toByteArray());
    }

    bool open(const QString& page) {
        const QString path = QDir(dir_).filePath(page);
        if (!QFileInfo::exists(path)) return false;
        browser_->setSource(QUrl::fromLocalFile(path));
        return true;
    }

protected:
    void closeEvent(QCloseEvent* e) override {
        QSettings s = appSettings();
        s.setValue(u"help/geometry"_s, saveGeometry());
        QWidget::closeEvent(e);
    }

private:
    // The CHM table of contents (toc_<lang>.hhc): nested <UL>/<LI> with <param name="Name"> and <param name="Local">.
    void loadToc(const QString& lang) {
        QFile f(QDir(dir_).filePath(u"toc_%1.hhc"_s.arg(lang)));
        if (!f.open(QIODevice::ReadOnly) && !(f.setFileName(QDir(dir_).filePath(u"toc_en.hhc"_s)), f.open(QIODevice::ReadOnly))) return;
        const QByteArray raw = f.readAll();
        const QString text = raw.startsWith("\xEF\xBB\xBF") ? QString::fromUtf8(raw.mid(3)) : QString::fromUtf8(raw);
        static const QRegularExpression token(u"<(/?)ul>|<param name=\"(Name|Local)\" value=\"([^\"]*)\">"_s, QRegularExpression::CaseInsensitiveOption);
        QList<QTreeWidgetItem*> stack;
        QTreeWidgetItem *last = nullptr, *root = toc_->invisibleRootItem();
        stack << root;
        QString pending;
        auto it = token.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            if (m.captured(2).isEmpty()) {  // <UL> or </UL>
                if (m.captured(1).isEmpty()) stack << (last ? last : stack.last());
                else if (stack.size() > 1) stack.removeLast();
            } else if (m.captured(2).compare(u"Name", Qt::CaseInsensitive) == 0) {
                pending = QTextDocumentFragment::fromHtml(m.captured(3)).toPlainText();
            } else {
                last = new QTreeWidgetItem(stack.last(), {pending});
                last->setData(0, Qt::UserRole, m.captured(3));
            }
        }
        toc_->expandAll();
    }

    void selectInToc(const QString& page) {
        QTreeWidgetItemIterator it(toc_);
        while (*it) {
            if ((*it)->data(0, Qt::UserRole).toString() == page) {
                QSignalBlocker block(toc_);
                toc_->setCurrentItem(*it);
                return;
            }
            ++it;
        }
    }

    QString dir_;
    QTreeWidget* toc_;
    QTextBrowser* browser_;
};

QPointer<HelpWindow> g_window;

}  // namespace

void showHelp(const QString& topic, QWidget* parent) {
    const QString dir = dataPath(u"help"_s);
    const QString lang = helpLanguageCode();
    const QString page = u"%1_%2.html"_s.arg(topic, lang);
    if (!QFileInfo::exists(QDir(dir).filePath(page))) {
        // Say exactly where the program looked: installation problems are then obvious.
        const QMessageBox::StandardButton b = QMessageBox::warning(
            parent, QApplication::applicationName(),
            QApplication::translate("Ren", "The help files were not found in:\n%1\n\nOpen the program's web site instead?").arg(QDir::toNativeSeparators(dir)),
            QMessageBox::Yes | QMessageBox::No);
        if (b == QMessageBox::Yes) QDesktopServices::openUrl(QUrl(u"https://www.antp.be/software/renamer/"_s));
        return;
    }
    if (!g_window) g_window = new HelpWindow(dir, lang, parent);
    g_window->open(page);
    g_window->show();
    g_window->raise();
    g_window->activateWindow();
}
