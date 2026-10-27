#include "helpwindow.h"

#include "appsettings.h"
#include "lngtranslator.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHash>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSplitter>
#include <QStyle>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace {

// Markdown headings are written "## Title {#id}", which QTextBrowser does not know: the "{#id}" is cut out of the text
// before it is shown, and a link to "page.md#id" (or "#id") scrolls to the heading by hand.
class MdBrowser : public QTextBrowser {
public:
    using QTextBrowser::QTextBrowser;

protected:
    QVariant loadResource(int type, const QUrl& name) override {
        QVariant v = QTextBrowser::loadResource(type, name);
        if (type != QTextDocument::MarkdownResource) {
            return v;
        }
        QString text = v.userType() == QMetaType::QByteArray ? QString::fromUtf8(v.toByteArray()) : v.toString();
        static const QRegularExpression tag(u"^(#+ .*?) *{#([\\w-]+)}$"_s, QRegularExpression::MultilineOption);
        titles_.clear();
        for (auto it = tag.globalMatch(text); it.hasNext();) {
            const auto m = it.next();
            titles_.insert(m.captured(2), m.captured(1).section(u' ', 1));
        }
        return text.replace(tag, u"\\1"_s);
    }

    void doSetSource(const QUrl& url, QTextDocument::ResourceType type) override {
        QTextBrowser::doSetSource(url, type);
        const QString title = titles_.value(url.fragment());
        if (title.isEmpty()) {
            return;
        }
        QTimer::singleShot(0, this, [this, title] {  // after the layout
            for (QTextBlock b = document()->begin(); b.isValid(); b = b.next()) {
                if (b.blockFormat().headingLevel() && b.text() == title) {
                    verticalScrollBar()->setValue(int(document()->documentLayout()->blockBoundingRect(b).top()));
                    return;
                }
            }
        });
    }

private:
    QHash<QString, QString> titles_;  // anchor id -> heading text of the page shown
};

class HelpWindow : public QWidget {
public:
    HelpWindow(const QString& dir, const QString& lang, QWidget* parent) : QWidget(parent, Qt::Window), dir_(dir) {
        setAttribute(Qt::WA_DeleteOnClose, false);
        setWindowTitle(QApplication::translate("Ren", "Help") + u" - "_s + QApplication::applicationName());
        resize(1040, 650);  // wide enough for the 680 px screenshots of the pages

        toc_ = new QTreeWidget;
        toc_->setHeaderHidden(true);
        browser_ = new MdBrowser;
        browser_->setOpenExternalLinks(true);
        browser_->setSearchPaths({dir_});

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
        split->setSizes({250, 790});
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(4, 4, 4, 4);
        l->addWidget(split);

        loadToc(lang);
        connect(toc_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* it) {
            const QString page = it->data(0, Qt::UserRole).toString();
            if (!page.isEmpty()) {
                open(page);
            }
        });
        connect(browser_, &QTextBrowser::sourceChanged, this,
                [this](const QUrl& u) { selectInToc(QFileInfo(u.toLocalFile()).fileName()); });
        QSettings s = appSettings();
        if (s.contains(u"help/geometry"_s)) {
            restoreGeometry(s.value(u"help/geometry"_s).toByteArray());
        }
    }

    bool open(const QString& page) {
        const QString path = QDir(dir_).filePath(page);
        if (!QFileInfo::exists(path)) {
            return false;
        }
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
    // The table of contents (toc_<lang>.md): a nested list of "- [Title](page.md)", two spaces of indentation per level.
    void loadToc(const QString& lang) {
        QFile f(QDir(dir_).filePath(u"toc_%1.md"_s.arg(lang)));
        if (!f.open(QIODevice::ReadOnly) && !(f.setFileName(QDir(dir_).filePath(u"toc_en.md"_s)), f.open(QIODevice::ReadOnly))) {
            return;
        }
        static const QRegularExpression entry(u"^( *)- \\[(.*)\\]\\((.*)\\)\\s*$"_s);
        QList<QTreeWidgetItem*> parents;  // parents[d] = the last item of depth d
        for (const QString& line : QString::fromUtf8(f.readAll()).split(u'\n')) {
            const auto m = entry.match(line);
            if (!m.hasMatch()) {
                continue;
            }
            const qsizetype depth = qMin<qsizetype>(m.capturedLength(1) / 2, parents.size());
            auto* it = new QTreeWidgetItem(depth ? parents[depth - 1] : toc_->invisibleRootItem(), {m.captured(2)});
            it->setData(0, Qt::UserRole, m.captured(3));
            parents.resize(depth);
            parents << it;
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
    MdBrowser* browser_;
};

QPointer<HelpWindow> g_window;

}  // namespace

void showHelp(const QString& topic, QWidget* parent) {
    const QString dir = dataPath(u"help"_s);
    const QString lang = helpLanguageCode();
    const QString page = u"%1_%2.md"_s.arg(topic, lang);
    if (!QFileInfo::exists(QDir(dir).filePath(page))) {
        // Say exactly where the program looked: installation problems are then obvious.
        const QMessageBox::StandardButton b = QMessageBox::warning(
            parent, QApplication::applicationName(),
            QApplication::translate("Ren", "The help files were not found in:\n%1\n\nOpen the program's web site instead?")
                .arg(QDir::toNativeSeparators(dir)),
            QMessageBox::Yes | QMessageBox::No);
        if (b == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl(u"https://www.antp.be/software/renamer/"_s));
        }
        return;
    }
    if (!g_window) {
        g_window = new HelpWindow(dir, lang, parent);
    }
    g_window->open(page);
    g_window->show();
    g_window->raise();
    g_window->activateWindow();
}
