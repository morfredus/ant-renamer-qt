#include "mainwindow.h"

#include "appsettings.h"
#include "icons.h"
#include "lngtranslator.h"
#include "theme.h"
#include "ren_path.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLocale>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QCollator>
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

namespace {

constexpr int SortRole = Qt::UserRole + 1;
constexpr int PtrRole = Qt::UserRole;

QString native(const QString& p) { return QDir::toNativeSeparators(p); }

QString tr_(const char* s) { return QCoreApplication::translate("Ren", s); }

// Sorts like the original: text case-insensitively, size and dates numerically.
class FileItem : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem& o) const override {
        const int c = treeWidget()->sortColumn();
        const QVariant a = data(c, SortRole), b = o.data(c, SortRole);
        if (a.isValid() && b.isValid()) return a.toLongLong() < b.toLongLong();
        static QCollator natural = [] { QCollator q; q.setNumericMode(true); q.setCaseSensitivity(Qt::CaseInsensitive); return q; }();
        if (appOptions().naturalSort) return natural.compare(text(c), o.text(c)) < 0;
        return text(c).compare(o.text(c), Qt::CaseInsensitive) < 0;
    }
};

QColor folderColor(const QPalette& p) {  // window colour darkened by 8 per channel, like the original
    const QColor base = p.color(QPalette::Base);
    return QColor(qMax(0, base.red() - 8), qMax(0, base.green() - 8), qMax(0, base.blue() - 8));
}

}  // namespace

// --- ItemTree ------------------------------------------------------------------------------------
ItemTree::ItemTree(bool acceptUrls) : acceptUrls_(acceptUrls) {
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setSelectionMode(ExtendedSelection);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(InternalMove);
    setDropIndicatorShown(true);
}

void ItemTree::dragEnterEvent(QDragEnterEvent* e) {
    if (acceptUrls_ && e->mimeData()->hasUrls()) e->acceptProposedAction();
    else QTreeWidget::dragEnterEvent(e);
}
void ItemTree::dragMoveEvent(QDragMoveEvent* e) {
    if (acceptUrls_ && e->mimeData()->hasUrls()) e->acceptProposedAction();
    else QTreeWidget::dragMoveEvent(e);
}
void ItemTree::dropEvent(QDropEvent* e) {
    if (e->mimeData()->hasUrls()) {
        if (!acceptUrls_) return;
        QStringList paths;
        for (const QUrl& u : e->mimeData()->urls())
            if (u.isLocalFile()) paths << native(u.toLocalFile());
        e->acceptProposedAction();
        emit urlsDropped(paths);
        return;
    }
    QTreeWidget::dropEvent(e);
    emit reordered();
}

// --- RenWorker ----------------------------------------------------------------------------------------
void RenWorker::run() {
    try {
        actions_->perform(*files_, op_, [this](File& f, int batchPos, int idx) {
            emit progress({quintptr(&f), f.description(), int(f.status()), batchPos, idx, f.fileFolder(), f.fileName(), f.preview});
        });
    } catch (const std::exception& e) {
        emit progress({0, QStringLiteral("Thread error caught: ") + QString::fromLocal8Bit(e.what()), int(State::Error), 0, 0});
    }
}

// --- construction -------------------------------------------------------------------------------------
QAction* MainWindow::makeAction(const QString& text, const QString& hint, int icon, const QKeySequence& key) {
    auto* a = new QAction(tbIcon(icon), text, this);
    a->setToolTip(hint);
    a->setData(icon);  // remembered so applyIcons() can switch icon set at run time
    if (!key.isEmpty()) a->setShortcut(key);
    a->setShortcutContext(Qt::WindowShortcut);
    addAction(a);  // keeps shortcuts alive when the toolbar button is hidden
    return a;
}

MainWindow::MainWindow() {
    qRegisterMetaType<Snapshot>("Snapshot");
    setWindowTitle(tr_("Ant Renamer Qt %1").arg(QApplication::applicationVersion()));
    setAcceptDrops(true);
    setMinimumSize(300, 300);
    resize(730, 500);

    appOptions().load();
    newUi_ = appOptions().newUi;
    theme_ = appOptions().theme;
    applyTheme(theme_, newUi_);  // the modern layout uses Fusion, which follows the system palette, light or dark
    pageLayout_ = newUi_ ? qBound(0, appOptions().pageLayout, 2) : 0;
    singleScreen_ = pageLayout_ > 0;
    singleMode_ = pageLayout_ == 2 ? 2 : 1;
    if (newUi_) {
        const QSize room = QGuiApplication::primaryScreen()->availableGeometry().size() - QSize(40, 60);
        resize((pageLayout_ == 2 ? QSize(1500, 850) : singleScreen_ ? QSize(1280, 800) : QSize(980, 640)).boundedTo(room));
    }
    setIconSet(appOptions().iconSet);
    appOptions().applyFont();
    createPages();
    createActions();
    createToolbars();
    if (singleScreen_) {
        buildSingleScreen();
        // keep the Move buttons on the list that has the focus (there is no current tab any more)
        connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* now) {
            for (int i = 0; i < 2 && now; ++i)
                if (panes_[i]->isAncestorOf(now)) currentTab_ = i;
        });
    }
    createStatusBar();
    // Flat toolbar buttons with hover / pressed / checked feedback, like the original TBX toolbars.
    // rgba() so it works on light and dark themes.
    if (newUi_) setStyleSheet(uR"(
        QToolBar { spacing: 2px; padding: 4px 6px; border: 0; border-bottom: 1px solid palette(mid); }
        QToolButton { border: 1px solid transparent; border-radius: 4px; padding: 4px 8px; background: transparent; }
        QToolButton:hover { background: palette(midlight); border-color: palette(mid); }
        QToolButton:pressed { background: palette(mid); }
        QToolButton:checked { border-radius: 0; border-bottom: 2px solid palette(highlight); font-weight: bold; background: transparent; }
        QToolButton:disabled { background: transparent; border-color: transparent; }
        QToolButton::menu-button { border: 0; width: 12px; }
        QToolButton::menu-indicator { image: none; }
        QToolButton#goButton { background: palette(highlight); color: palette(highlighted-text); font-weight: bold; padding: 4px 16px; }
        QToolButton#goButton:hover { background: palette(highlight); border-color: palette(highlighted-text); }
        QToolButton#goButton:disabled { background: palette(midlight); color: palette(mid); }
        QGroupBox#preview::indicator { width: 0; height: 0; }
        QSplitter::handle:horizontal { width: 5px; }
        QSplitter::handle:vertical { height: 5px; }
        QFrame#editBar { border: 1px solid palette(highlight); border-radius: 4px; }
        QGroupBox { font-weight: bold; margin-top: 10px; padding-top: 6px; }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 4px; }
        QGroupBox QLabel, QGroupBox QTreeWidget { font-weight: normal; }
    )"_s);
    else setStyleSheet(uR"(
        QToolBar { spacing: 1px; padding: 1px; border: 0; }
        QToolButton { border: 1px solid transparent; border-radius: 2px; padding: 2px 3px; background: transparent; }
        QToolButton:hover { background: rgba(0,120,215,40); border-color: rgba(0,120,215,110); }
        QToolButton:pressed { background: rgba(0,120,215,90); border-color: rgba(0,120,215,170); padding: 3px 2px 1px 4px; }
        QToolButton:checked { background: rgba(0,120,215,70); border-color: rgba(0,120,215,150); font-weight: bold; }
        QToolButton:checked:hover { background: rgba(0,120,215,90); }
        QToolButton:disabled { background: transparent; border-color: transparent; }
        QToolButton::menu-button { border: 0; width: 12px; }
    )"_s);
    loadSettings();
    setStatus();
    showTab(0);
    aStop_->setEnabled(false);
}

void MainWindow::createActions() {
    using K = QKeySequence;
    aGo_ = makeAction(tr_("Go"), tr_("Start the renaming process"), TbGo, K(Qt::Key_F9));
    aStop_ = makeAction(tr_("Stop"), tr_("Stop the renaming process"), TbStop, K(Qt::Key_Escape));
    aUndo_ = makeAction(tr_("Undo"), tr_("Undo last renaming"), TbUndo, K(Qt::Key_F12));
    aOnTop_ = makeAction(tr_("On top"), tr_("Keep window always on top"), TbAlwaysOnTop, K(Qt::Key_F11));
    aOnTop_->setCheckable(true);
    aTheme_ = makeAction(tr_("Light / dark"), tr_("Switch between the light and the dark theme"), TbTheme);
    aOptions_ = makeAction(tr_("Options..."), tr_("Change program options"), TbOptions, K(Qt::Key_F10));
    aHelp_ = makeAction(tr_("Help"), tr_("Open help contents"), TbHelp, K(Qt::Key_F1));
    aAbout_ = makeAction(tr_("About..."), tr_("About this program"), TbAbout, K(Qt::SHIFT | Qt::Key_F1));
    aExit_ = makeAction(tr_("Exit"), tr_("Quit the program"), TbExit, K(Qt::ALT | Qt::Key_F4));

    const char* tabNames[] = {"Files", "Actions", "Log"};
    const char* tabHints[] = {"Display files", "Display actions", "Display log"};
    for (int i = 0; i < 3; ++i) {
        aTab_[i] = makeAction(tr_(tabNames[i]), tr_(tabHints[i]), TbTabFiles + i, K(Qt::CTRL | (Qt::Key_1 + i)));
        aTab_[i]->setCheckable(true);
        connect(aTab_[i], &QAction::triggered, this, [this, i](bool on) {
            if (singleScreen_) setPaneVisible(i, on);  // the page buttons show / hide a pane
            else showTab(i);
        });
    }

    aAddFiles_ = makeAction(tr_("Add files..."), tr_("Add files to the list"), TbAddFiles, K(Qt::Key_F2));
    aAddFolders_ = makeAction(tr_("Add folders..."), tr_("Add folders or folder contents to the list"), TbAddFolder, K(Qt::Key_F3));
    aRemoveSel_ = makeAction(tr_("Remove sel."), tr_("Remove selected files from the list"), TbRemoveSelected, K(Qt::Key_F6));
    aRemoveAll_ = makeAction(tr_("Remove all"), tr_("Remove all files from the list"), TbRemoveAll, K(Qt::Key_F7));
    aRemoveDead_ = makeAction(tr_("Remove dead"), tr_("Remove dead files from the list"), TbRemoveDead, K(Qt::Key_F8));
    aPreview_ = makeAction(tr_("Preview"), tr_("Refresh preview"), TbPreview, K(Qt::Key_F5));

    // view filter only: hidden files stay in the list and are processed as usual
    aChangedOnly_ = makeAction(tr_("Changed only"), tr_("Show only the files whose name changes in the preview"), TbLogFilter);
    aChangedOnly_->setIcon(QIcon());
    aChangedOnly_->setData(QVariant());
    aChangedOnly_->setCheckable(true);
    connect(aChangedOnly_, &QAction::toggled, this, [this] { updateAllItems(); });

    aBatchAdd_ = makeAction(tr_("Add to batch"), tr_("Add to batch"), TbBatchAdd, K(Qt::CTRL | Qt::Key_F2));
    aBatchEdit_ = makeAction(tr_("Modify"), tr_("Replace the selected action with the current settings (double-click an action to load its settings)"), TbBatchAdd);
    aBatchEdit_->setEnabled(false);
    connect(aBatchEdit_, &QAction::triggered, this, [this] { updateBatchItem(); });
    if (newUi_) {
        connect(batchView_, &QTreeWidget::itemSelectionChanged, this, [this] { aBatchEdit_->setEnabled(!worker_ && batchView_->selectedItems().size() == 1); });
        connect(batchView_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* it, int) { editBatchItem(it); });
    }
    aBatchRemoveSel_ = makeAction(tr_("Remove sel."), tr_("Remove selected actions from the batch"), TbBatchRemoveSel, K(Qt::SHIFT | Qt::Key_F6));
    aBatchClear_ = makeAction(tr_("Clear"), tr_("Remove all actions from the batch"), TbBatchClear, K(Qt::SHIFT | Qt::Key_F7));
    aBatchOpen_ = makeAction(tr_("Open..."), tr_("Open a batch file"), TbBatchOpen, K(Qt::CTRL | Qt::Key_O));
    aBatchSave_ = makeAction(tr_("Save..."), tr_("Save this batch to a file"), TbBatchSave, K(Qt::CTRL | Qt::Key_S));

    aLogFilter_ = makeAction(tr_("Filter"), tr_("Filter log entries"), TbLogFilter);
    aLogOk_ = makeAction(tr_("Successes"), tr_("Show renamed files"), TbLogFilter);
    aLogError_ = makeAction(tr_("Errors"), tr_("Show entries corresponding to errors"), TbLogFilter);
    aLogNot_ = makeAction(tr_("Not renamed"), tr_("Show skipped files"), TbLogFilter);
    for (QAction* a : {aLogOk_, aLogError_, aLogNot_}) {
        a->setIcon(QIcon());
        a->setData(QVariant());
        a->setCheckable(true);
        a->setChecked(true);
        connect(a, &QAction::toggled, this, [this] {
            logShowOk_ = aLogOk_->isChecked();
            logShowError_ = aLogError_->isChecked();
            logShowNot_ = aLogNot_->isChecked();
            rebuildLog();
        });
    }
    aLogClear_ = makeAction(tr_("Clear"), tr_("Clear log"), TbLogClear);
    aLogSave_ = makeAction(tr_("Save..."), tr_("Save log to a file"), TbLogSave);
    aLogCopy_ = makeAction(tr_("Copy"), tr_("Copy log to clipboard"), TbLogCopy);

    aUp_ = makeAction(tr_("Up"), tr_("Move up selected items"), TbMoveUp, K(Qt::CTRL | Qt::Key_Up));
    aDown_ = makeAction(tr_("Down"), tr_("Move down selected items"), TbMoveDown, K(Qt::CTRL | Qt::Key_Down));
    aTop_ = makeAction(tr_("Top"), tr_("Move selected items to top of the list"), TbMoveTop, K(Qt::CTRL | Qt::SHIFT | Qt::Key_Up));
    aBottom_ = makeAction(tr_("Bottom"), tr_("Move selected items to bottom of the list"), TbMoveBottom, K(Qt::CTRL | Qt::SHIFT | Qt::Key_Down));

    aSelAll_ = new QAction(tr_("Select all"), this);
    aSelNone_ = new QAction(tr_("Select none"), this);
    aSelInvert_ = new QAction(tr_("Invert selection"), this);
    aUnsort_ = new QAction(tr_("Unsorted items"), this);

    // wiring
    connect(aGo_, &QAction::triggered, this, [this] { run(Op::Rename); });
    connect(aStop_, &QAction::triggered, this, [] { stopFlag() = true; });
    connect(aUndo_, &QAction::triggered, this, [this] { run(Op::Revert); });
    connect(aPreview_, &QAction::triggered, this, [this] { run(Op::Preview); });
    connect(aOnTop_, &QAction::toggled, this, [this](bool on) {
        const bool vis = isVisible();
        setWindowFlag(Qt::WindowStaysOnTopHint, on);
        if (vis) show();
    });
    connect(aTheme_, &QAction::triggered, this, [this] {
        appOptions().theme = isDarkPalette(QGuiApplication::palette()) ? 1 : 2;
        appOptions().save();
        applyOptionsLive();
    });
    connect(aOptions_, &QAction::triggered, this, [this] {
        OptionsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) applyOptionsLive();
    });
    connect(aHelp_, &QAction::triggered, this, [] { openHelp(u"index"_s); });
    connect(panel_, &ActionPanel::helpRequested, this, [](const QString& topic) { openHelp(topic); });
    connect(aAbout_, &QAction::triggered, this, [this] { AboutDialog(this).exec(); });
    connect(aExit_, &QAction::triggered, this, &QWidget::close);

    connect(aAddFiles_, &QAction::triggered, this, [this] {
        QSettings s = appSettings();
        const QStringList f = QFileDialog::getOpenFileNames(this, tr_("Select files"), s.value(u"folders/addFiles"_s).toString());
        if (f.isEmpty()) return;
        s.setValue(u"folders/addFiles"_s, QFileInfo(f.first()).absolutePath());
        QStringList n;
        for (const QString& p : f) n << native(p);
        addFiles(n, nullptr);
    });
    connect(aAddFolders_, &QAction::triggered, this, [this] {
        QSettings s = appSettings();
        AddFoldersDialog dlg(s.value(u"folders/addFolder"_s).toString(), this);
        if (dlg.exec() != QDialog::Accepted) return;
        const AddChoice c = dlg.choice();
        appOptions().addFiles = c.files;
        appOptions().addFolders = c.folders;
        appOptions().recursive = c.recursive;
        appOptions().save();
        if (!dlg.folders().isEmpty()) s.setValue(u"folders/addFolder"_s, dlg.folders().first());
        QStringList n;
        for (const QString& p : dlg.folders()) n << native(p);
        addFiles(n, &c);
    });
    connect(aRemoveSel_, &QAction::triggered, this, [this] { removeSelectedFiles(); });
    connect(aRemoveAll_, &QAction::triggered, this, [this] { removeAllFiles(); });
    connect(aRemoveDead_, &QAction::triggered, this, [this] { removeDeadFiles(); });

    connect(aBatchAdd_, &QAction::triggered, this, [this] { addToBatch(); });
    connect(aBatchRemoveSel_, &QAction::triggered, this, [this] { removeSelectedActions(); });
    connect(aBatchClear_, &QAction::triggered, this, [this] { clearBatch(); });
    connect(aBatchOpen_, &QAction::triggered, this, [this] { openBatch(); });
    connect(aBatchSave_, &QAction::triggered, this, [this] { saveBatch(); });

    connect(aLogClear_, &QAction::triggered, this, [this] { logAction(0); });
    connect(aLogSave_, &QAction::triggered, this, [this] { logAction(1); });
    connect(aLogCopy_, &QAction::triggered, this, [this] { logAction(2); });

    connect(aUp_, &QAction::triggered, this, [this] { moveItems(0); });
    connect(aDown_, &QAction::triggered, this, [this] { moveItems(1); });
    connect(aTop_, &QAction::triggered, this, [this] { moveItems(2); });
    connect(aBottom_, &QAction::triggered, this, [this] { moveItems(3); });

    connect(aSelAll_, &QAction::triggered, this, [this] { if (auto* t = currentList()) t->selectAll(); });
    connect(aSelNone_, &QAction::triggered, this, [this] { if (auto* t = currentList()) t->clearSelection(); });
    connect(aSelInvert_, &QAction::triggered, this, [this] {
        if (auto* t = currentList())
            for (int i = 0; i < t->topLevelItemCount(); ++i)
                if (!t->topLevelItem(i)->isHidden()) t->topLevelItem(i)->setSelected(!t->topLevelItem(i)->isSelected());
    });
    connect(aUnsort_, &QAction::triggered, this, [this] { unsortFiles(); });
}

// Toolbars. Every toolbar is described once, as a list of items; the classic layout puts the bars on the window, the
// modern layout puts one main bar on the window and each page bar inside its page. The buttons themselves are the
// shared QActions of createActions() (like the Delphi TActions), so a new button is added here once for both layouts.
void MainWindow::createToolbars() {
    enum Kind { Act, ActText, Sep, Spacer, RecentMenu, FilterMenu };
    struct Item {
        Kind kind;
        QAction* a = nullptr;
    };
    using Items = std::vector<Item>;
    auto A = [](QAction* a) { return Item{Act, a}; };      // icon only
    auto T = [](QAction* a) { return Item{ActText, a}; };  // text beside the icon (the buttons the original labelled)
    const Item sep{Sep}, spacer{Spacer}, recent{RecentMenu}, filter{FilterMenu};
    auto append = [](Items& to, const Items& more) { to.insert(to.end(), more.begin(), more.end()); };

    auto bar = [this](const QString& name, const Items& items) {
        auto* t = new QToolBar(name, this);
        t->setObjectName(name);
        t->setMovable(!newUi_);
        t->setIconSize(QSize(16, 16));
        t->setFloatable(false);
        for (const Item& i : items) {
            switch (i.kind) {
            case Act:
            case ActText:
                t->addAction(i.a);
                if (auto* b = qobject_cast<QToolButton*>(t->widgetForAction(i.a)))
                    b->setToolButtonStyle(i.kind == ActText ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
                break;
            case Sep:
                t->addSeparator();
                break;
            case Spacer: {
                auto* s = new QWidget;
                s->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                t->addWidget(s);
                break;
            }
            case RecentMenu: {  // "Open..." with the list of recent batch files
                recentMenu_ = new QMenu(this);
                auto* openBtn = new QToolButton;
                openBtn->setDefaultAction(aBatchOpen_);
                openBtn->setMenu(recentMenu_);
                openBtn->setPopupMode(QToolButton::MenuButtonPopup);
                t->addWidget(openBtn);
                connect(recentMenu_, &QMenu::aboutToShow, this, [this] { updateRecentMenu(); });
                break;
            }
            case FilterMenu: {  // log filter
                auto* filterMenu = new QMenu(this);
                filterMenu->addActions({aLogOk_, aLogError_, aLogNot_});
                auto* filterBtn = new QToolButton;
                filterBtn->setDefaultAction(aLogFilter_);
                filterBtn->setMenu(filterMenu);
                filterBtn->setPopupMode(QToolButton::InstantPopup);
                t->addWidget(filterBtn);
                break;
            }
            }
        }
        return t;
    };

    // --- what each bar contains ---
    const Items tabs = {T(aTab_[0]), T(aTab_[1]), T(aTab_[2])};
    const Items moves = {A(aUp_), A(aDown_), A(aTop_), A(aBottom_)};
    Items files = {T(aAddFiles_), T(aAddFolders_), sep, A(aRemoveSel_), T(aRemoveAll_), A(aRemoveDead_), sep, A(aPreview_), T(aChangedOnly_)};
    Items actions = {newUi_ ? T(aBatchAdd_) : A(aBatchAdd_)};
    if (newUi_) actions.push_back(T(aBatchEdit_));
    append(actions, {A(aBatchRemoveSel_), sep, A(aBatchClear_), recent, A(aBatchSave_)});
    const Items log = {filter, sep, A(aLogClear_), A(aLogSave_), A(aLogCopy_)};

    if (!newUi_) {  // classic: bars on the window, movable; the move buttons have their own bar
        tbTabs_ = bar(u"Pages"_s, tabs);
        addToolBar(Qt::TopToolBarArea, tbTabs_);
        tbProgram_ = bar(u"Program"_s, {T(aGo_), T(aStop_), A(aUndo_), sep, A(aOnTop_), A(aTheme_), A(aOptions_), sep, A(aHelp_), A(aAbout_), sep, A(aExit_)});
        addToolBar(Qt::TopToolBarArea, tbProgram_);
        addToolBarBreak();
        tbFiles_ = bar(u"Files"_s, files);
        addToolBar(Qt::TopToolBarArea, tbFiles_);
        tbActions_ = bar(u"Actions"_s, actions);
        addToolBar(Qt::TopToolBarArea, tbActions_);
        tbLog_ = bar(u"Log"_s, log);
        addToolBar(Qt::TopToolBarArea, tbLog_);
        tbMove_ = bar(u"Move Items"_s, moves);
        addToolBar(Qt::TopToolBarArea, tbMove_);
        return;
    }

    // modern: tabs + program buttons in one main bar, a slim bar (with the move buttons) in each page
    Items mainBar = tabs;
    append(mainBar, {spacer, T(aGo_), T(aStop_), A(aUndo_), A(aOptions_), A(aHelp_), sep, A(aOnTop_), A(aTheme_), A(aAbout_), A(aExit_)});
    tbProgram_ = bar(u"Main"_s, mainBar);
    if (auto* b = tbProgram_->widgetForAction(aGo_)) b->setObjectName(u"goButton"_s);  // the one coloured button
    addToolBar(Qt::TopToolBarArea, tbProgram_);
    for (Items* page : {&files, &actions}) {
        page->push_back(spacer);
        append(*page, moves);
    }
    tbFiles_ = bar(u"Files"_s, files);
    tbActions_ = bar(u"Actions"_s, actions);
    tbLog_ = bar(u"Log"_s, log);

    // each page gets its own bar on top
    int i = 0;
    for (auto sig : {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved})
        connect(batchView_->model(), sig, this, [this] {
            updateTabCounts();
            if (editBar_ && editBar_->isVisible() && !editIndex_.isValid()) endEdit();
        });
    for (QToolBar* t : {tbFiles_, tbActions_, tbLog_}) {
        QWidget* content = stack_->widget(i);
        stack_->removeWidget(content);
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        l->addWidget(t);
        l->addWidget(content, 1);
        content->show();  // removeWidget() hid it
        stack_->insertWidget(i++, w);
    }
}

void MainWindow::createPages() {
    stack_ = new QStackedWidget;
    setCentralWidget(stack_);

    // --- Files page
    fileView_ = new ItemTree(true);
    fileView_->setColumnCount(ColCount);
    fileView_->setHeaderLabels({tr_("Folder"), tr_("Filename"), tr_("Full path"), tr_("Ext."), tr_("Preview"), tr_("Size"),
                                tr_("Date created"), tr_("Date modified")});
    fileView_->setAllColumnsShowFocus(true);
    fileView_->setContextMenuPolicy(Qt::CustomContextMenu);
    fileView_->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    fileView_->header()->setSectionsClickable(true);
    fileView_->header()->setSectionsMovable(true);
    fileView_->header()->setStretchLastSection(true);  // the header bar fills the whole width, like the original
    const int widths[ColCount] = {200, 400, 400, 60, 200, 70, 130, 130};
    for (int c = 0; c < ColCount; ++c) fileView_->setColumnWidth(c, widths[c]);
    for (int c : {ColFullPath, ColExt, ColPreview, ColSize, ColCreated, ColModified})
        if (!(newUi_ && c == ColPreview)) fileView_->setColumnHidden(c, true);  // modern layout: new name visible
    if (newUi_) {  // current name, new name, folder
        auto* h = fileView_->header();
        h->moveSection(h->visualIndex(ColFolder), h->visualIndex(ColPreview));
    }
    stack_->addWidget(fileView_);

    connect(fileView_, &ItemTree::urlsDropped, this, &MainWindow::filesDropped);
    connect(fileView_, &QTreeWidget::itemSelectionChanged, this, [this] { updateSamplePreview(); });
    connect(fileView_->header(), &QHeaderView::sectionClicked, this, &MainWindow::sortFilesBy);
    connect(fileView_->header(), &QHeaderView::customContextMenuRequested, this, &MainWindow::fileHeaderMenu);
    connect(fileView_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* it, int) {
        if (appOptions().launchFile)
            if (File* f = fileOf(it)) QDesktopServices::openUrl(QUrl::fromLocalFile(f->filePath()));
    });
    connect(fileView_, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        QMenu m;
        m.addAction(aRemoveSel_);
        m.addAction(aRemoveAll_);
        m.addAction(aSelAll_);
        m.addAction(aSelNone_);
        m.addAction(aSelInvert_);
        m.addSeparator();
        m.addAction(aPreview_);
        m.exec(fileView_->viewport()->mapToGlobal(p));
    });

    // --- Actions page
    auto* page = new QWidget;
    auto* pl = new QVBoxLayout(page);
    pl->setContentsMargins(0, 0, 0, 0);
    actionsSplit_ = new QSplitter(newUi_ ? Qt::Horizontal : Qt::Vertical);
    actionsSplit_->setChildrenCollapsible(false);
    pl->addWidget(actionsSplit_);

    panel_ = new ActionPanel;
    if (!newUi_) actionsSplit_->addWidget(panel_);
    connect(panel_, &ActionPanel::changed, this, [this] { updateSamplePreview(); });

    previewGroup_ = new QGroupBox(tr_("Preview of selected file"));
    previewGroup_->setCheckable(true);
    previewGroup_->setChecked(true);
    previewGroup_->setMaximumHeight(80);
    auto* pg = new QVBoxLayout(previewGroup_);
    auto row = [&](const QString& label, QLabel*& value) {
        auto* h = new QHBoxLayout;
        auto* l = new QLabel(label);
        l->setMinimumWidth(110);
        value = new QLabel;
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        h->addWidget(l);
        h->addWidget(value, 1);
        pg->addLayout(h);
    };
    row(tr_("Current name:"), previewOld_);
    row(tr_("Sample new name:"), previewNew_);
    if (newUi_) {  // fixed strip under the plan and the editor
        previewGroup_->setObjectName(u"preview"_s);
        previewGroup_->setMaximumHeight(90);
        pl->addWidget(previewGroup_);
        connect(previewGroup_, &QGroupBox::toggled, this, [this](bool on) { if (!on) previewGroup_->setChecked(true); });  // always on
    } else {
        actionsSplit_->addWidget(previewGroup_);
    }
    connect(previewGroup_, &QGroupBox::toggled, this, [this](bool on) {
        previewOld_->setVisible(on);
        previewNew_->setVisible(on);
        for (auto* l : previewGroup_->findChildren<QLabel*>()) l->setVisible(on);
        if (on) updateSamplePreview();
    });

    batchGroup_ = new QGroupBox(tr_("Batch contents (planned actions)"));
    batchGroup_->setCheckable(!newUi_);  // modern layout: the plan is always visible
    if (newUi_) batchGroup_->setMinimumWidth(170);  // the long title must not decide the width of the plan
    if (!newUi_) batchGroup_->setChecked(false);
    auto* bg = new QVBoxLayout(batchGroup_);
    batchView_ = new ItemTree(false);
    batchView_->setColumnCount(2);
    batchView_->setHeaderHidden(true);
    batchView_->setColumnWidth(0, 30);
    batchView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    batchView_->setContextMenuPolicy(Qt::CustomContextMenu);
    bg->addWidget(batchView_);
    actionsSplit_->addWidget(batchGroup_);
    if (newUi_) {  // plan on the left, editor on the right
        auto* right = new QWidget;
        auto* rl = new QVBoxLayout(right);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->addWidget(panel_, 1);
        editBar_ = new QFrame;
        editBar_->setObjectName(u"editBar"_s);
        auto* eh = new QHBoxLayout(editBar_);
        editLabel_ = new QLabel;
        auto* okEdit = new QPushButton(tr_("Validate the modification"));
        auto* cancelEdit = new QPushButton(tr_("Cancel the modification"));
        eh->addWidget(editLabel_, 1);
        eh->addWidget(okEdit);
        eh->addWidget(cancelEdit);
        editBar_->hide();
        rl->addWidget(editBar_);
        connect(okEdit, &QPushButton::clicked, this, [this] { validateEdit(); });
        connect(cancelEdit, &QPushButton::clicked, this, [this] { endEdit(); });
        actionsSplit_->addWidget(right);
        actionsSplit_->setStretchFactor(1, 1);
        actionsSplit_->setSizes({280, 700});
    } else {
        actionsSplit_->setStretchFactor(0, 1);
    }
    connect(batchGroup_, &QGroupBox::toggled, this, [this](bool on) { setBatchVisible(on); });
    connect(batchView_, &ItemTree::reordered, this, [this] { renumberBatchView(); });
    connect(batchView_, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        QMenu m;
        if (newUi_) m.addAction(aBatchEdit_);
        m.addAction(aBatchRemoveSel_);
        m.addAction(aBatchClear_);
        m.exec(batchView_->viewport()->mapToGlobal(p));
    });
    stack_->addWidget(page);

    // --- Log page
    logView_ = new QListWidget;
    logView_->setSelectionMode(QAbstractItemView::NoSelection);
    logView_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(logView_, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        QMenu m;
        m.addAction(aLogClear_);
        m.addAction(aLogCopy_);
        m.exec(logView_->viewport()->mapToGlobal(p));
    });
    stack_->addWidget(logView_);

    panel_->loadState();
}

void MainWindow::createStatusBar() {
    progress_ = new QProgressBar;
    progress_->setFixedWidth(146);
    progress_->setRange(0, 100);
    progress_->setTextVisible(true);
    countLabel_ = new QLabel;
    countLabel_->setMinimumWidth(70);
    countLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_ = new QLabel;
    statusBar()->addWidget(progress_);
    statusBar()->addWidget(countLabel_);
    statusBar()->addWidget(statusLabel_, 1);
}

// --- settings -------------------------------------------------------------------------------------------
void MainWindow::loadSettings() {
    QSettings s = appSettings();
    // the modern layout keeps its own geometry, columns and splitter (the old ones do not fit it)
    const QString sfx = newUi_ ? (singleScreen_ ? u"Single"_s : u"New"_s) : QString();
    if (singleScreen_) {
        stBands_ = s.value(u"window/singleBands"_s).toByteArray();
        stColsOuter_ = s.value(u"window/singleColsOuter"_s).toByteArray();
        stColsInner_ = s.value(u"window/singleColsInner"_s).toByteArray();
        const int mask = s.value(u"window/panes"_s, 7).toInt();
        for (int i = 0; i < 3; ++i) paneVisible_[i] = mask & (1 << i);
        arrangeSingle(singleMode_);  // applies the remembered sizes
        for (int i = 0; i < 3; ++i) aTab_[i]->setChecked(paneVisible_[i]);
    }
    if (s.contains(u"window/geometry"_s + sfx)) restoreGeometry(s.value(u"window/geometry"_s + sfx).toByteArray());
    if (s.contains(u"window/filesHeader"_s + sfx)) fileView_->header()->restoreState(s.value(u"window/filesHeader"_s + sfx).toByteArray());
    if (s.contains(u"window/actionsSplit"_s + sfx)) actionsSplit_->restoreState(s.value(u"window/actionsSplit"_s + sfx).toByteArray());
    if (!newUi_) {
        previewGroup_->setChecked(s.value(u"window/enableSample"_s, true).toBool());
        batchGroup_->setChecked(s.value(u"window/enableBatch"_s, false).toBool());
        setBatchVisible(batchGroup_->isChecked());
    }
    aOnTop_->setChecked(s.value(u"window/onTop"_s, false).toBool());
    aChangedOnly_->setChecked(s.value(u"view/changedOnly"_s, false).toBool());
    aLogOk_->setChecked(s.value(u"log/ok"_s, true).toBool());
    aLogError_->setChecked(s.value(u"log/error"_s, true).toBool());
    aLogNot_->setChecked(s.value(u"log/not"_s, true).toBool());
    if (!newUi_ && s.contains(u"window/toolbars"_s)) restoreState(s.value(u"window/toolbars"_s).toByteArray());
    applyOptionsLive();
}

// Options that can change while the program runs (the others are read when used).
void MainWindow::applyOptionsLive() {
    const AppOptions& o = appOptions();
    if (o.theme != theme_) {
        theme_ = o.theme;
        applyTheme(theme_, newUi_);
        // The widgets under a style sheet keep the palette they were polished with: clear and set the sheet again.
        const QString sheet = styleSheet();
        setStyleSheet(QString());
        setStyleSheet(sheet);
        // A label with links keeps the link colour it was laid out with (dark blue on a dark background): lay it out again.
        for (QLabel* l : findChildren<QLabel*>())
            if (const QString html = l->text(); html.contains(u"<a "_s)) {  // (setText ignores the same text)
                l->clear();
                l->setText(html);
            }
        rebuildLog();  // the colours of the lines depend on the theme
    }
    setIconSet(o.iconSet);
    for (QAction* a : findChildren<QAction*>()) {
        bool ok = false;
        const int idx = a->data().toInt(&ok);
        if (ok && a->data().isValid()) a->setIcon(tbIcon(idx));
    }
    for (QToolBar* t : findChildren<QToolBar*>()) t->setIconSize(QSize(iconCell(), iconCell()));
    o.applyFont();
    fileView_->header()->setSectionResizeMode(o.resizeCols ? QHeaderView::Stretch : QHeaderView::Interactive);
    panel_->applyOptions();
    updateAllItems();
    setStatus();
}

void MainWindow::saveSettings() {
    QSettings s = appSettings();
    const QString sfx = newUi_ ? (singleScreen_ ? u"Single"_s : u"New"_s) : QString();
    if (singleScreen_) {
        saveSingleState();
        s.setValue(u"window/singleBands"_s, stBands_);
        s.setValue(u"window/singleColsOuter"_s, stColsOuter_);
        s.setValue(u"window/singleColsInner"_s, stColsInner_);
        s.setValue(u"window/panes"_s, int(paneVisible_[0]) | int(paneVisible_[1]) << 1 | int(paneVisible_[2]) << 2);
    }
    s.setValue(u"window/geometry"_s + sfx, saveGeometry());
    s.setValue(u"window/filesHeader"_s + sfx, fileView_->header()->saveState());
    s.setValue(u"window/actionsSplit"_s + sfx, actionsSplit_->saveState());
    if (!newUi_) {
        s.setValue(u"window/toolbars"_s, saveState());
        s.setValue(u"window/enableSample"_s, previewGroup_->isChecked());
        s.setValue(u"window/enableBatch"_s, batchGroup_->isChecked());
    }
    s.setValue(u"window/onTop"_s, aOnTop_->isChecked());
    s.setValue(u"view/changedOnly"_s, aChangedOnly_->isChecked());
    s.setValue(u"log/ok"_s, aLogOk_->isChecked());
    s.setValue(u"log/error"_s, aLogError_->isChecked());
    s.setValue(u"log/not"_s, aLogNot_->isChecked());
    panel_->saveState();
    appOptions().save();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (worker_) {
        QMessageBox::information(this, windowTitle(), tr_("Application is busy: Renaming operation in progress."));
        e->ignore();
        return;
    }
    saveSettings();
    e->accept();
}

// --- navigation / status -----------------------------------------------------------------------------------
void MainWindow::showTab(int tab) {
    currentTab_ = tab;
    if (singleScreen_) {  // everything is on screen: just make sure that pane is not hidden
        setPaneVisible(tab, true);
        return;
    }
    stack_->setCurrentIndex(tab);
    for (int i = 0; i < 3; ++i) aTab_[i]->setChecked(i == tab);
    if (newUi_) return;  // modern layout: each page carries its own bar
    tbActions_->setVisible(tab == 1);
    tbLog_->setVisible(tab == 2);
    tbFiles_->setVisible(true);  // the original keeps the files toolbar on every page
    tbMove_->setVisible((tab == 1 && batchGroup_->isChecked()) || tab == 0);
}

// --- modern layout: single screen ---------------------------------------------------------------------------------
// The pages built by createPages()/createToolbars() are three widgets (bar + content). Instead of a stack of tabs they go
// into splitters: bands (Files / Actions / Log stacked) or columns (Files on the left, Actions over Log on the right).
void MainWindow::buildSingleScreen() {
    for (QWidget*& p : panes_) {
        p = stack_->widget(0);
        stack_->removeWidget(p);
    }
    panes_[0]->setMinimumSize(300, 120);
    panes_[1]->setMinimumSize(380, 220);
    panes_[2]->setMinimumSize(200, 80);
    actionsSplit_->setSizes({190, 560});  // less room for the plan: the editor needs it (a saved state replaces this)
    singleHost_ = new QWidget;
    auto* l = new QVBoxLayout(singleHost_);
    l->setContentsMargins(0, 0, 0, 0);
    arrangeSingle(singleMode_);  // moves the panes out of the stack first
    setCentralWidget(singleHost_);  // deletes the (now empty) stack
    stack_ = nullptr;
}

void MainWindow::arrangeSingle(int mode) {
    for (QWidget* p : panes_) p->setParent(singleHost_);  // out of the old splitters, which are deleted below
    delete outer_;                                        // also deletes inner_
    outer_ = inner_ = nullptr;
    singleMode_ = mode;
    auto splitter = [](Qt::Orientation o) {
        auto* s = new QSplitter(o);
        s->setChildrenCollapsible(false);  // a pane is hidden with its page button, never lost by a drag
        return s;
    };
    if (mode == 2) {
        outer_ = splitter(Qt::Horizontal);
        inner_ = splitter(Qt::Vertical);
        outer_->addWidget(panes_[0]);
        outer_->addWidget(inner_);
        inner_->addWidget(panes_[1]);
        inner_->addWidget(panes_[2]);
    } else {
        outer_ = splitter(Qt::Vertical);
        for (QWidget* p : panes_) outer_->addWidget(p);
    }
    singleHost_->layout()->addWidget(outer_);
    for (int i = 0; i < 3; ++i) panes_[i]->setVisible(paneVisible_[i]);
    if (mode == 2) {
        outer_->setStretchFactor(0, 1);
        outer_->setStretchFactor(1, 1);
        inner_->setStretchFactor(0, 3);
        inner_->setStretchFactor(1, 1);
        if (!stColsOuter_.isEmpty()) outer_->restoreState(stColsOuter_); else outer_->setSizes({420, 880});
        if (!stColsInner_.isEmpty()) inner_->restoreState(stColsInner_); else inner_->setSizes({480, 180});
    } else {
        outer_->setStretchFactor(0, 2);
        outer_->setStretchFactor(1, 3);
        outer_->setStretchFactor(2, 1);
        if (!stBands_.isEmpty()) outer_->restoreState(stBands_); else outer_->setSizes({260, 320, 140});
    }
}

void MainWindow::saveSingleState() {
    if (!outer_) return;
    if (singleMode_ == 2) {
        stColsOuter_ = outer_->saveState();
        stColsInner_ = inner_->saveState();
    } else {
        stBands_ = outer_->saveState();
    }
}

void MainWindow::setPaneVisible(int page, bool on) {
    paneVisible_[page] = on;
    panes_[page]->setVisible(on);
    aTab_[page]->setChecked(on);
}

// Columns need room: below about 900 px they give way to bands, and come back above 960 px (no flapping).
void MainWindow::resizeEvent(QResizeEvent* e) {
    QMainWindow::resizeEvent(e);
    if (!singleScreen_ || pageLayout_ != 2 || !outer_) return;
    const int want = singleMode_ == 2 ? (width() < 900 ? 1 : 2) : (width() >= 960 ? 2 : 1);
    if (want != singleMode_) {
        saveSingleState();
        arrangeSingle(want);
    }
}

void MainWindow::setBatchVisible(bool on) {
    if (newUi_) return;  // the plan is always visible
    batchView_->setVisible(on);
    batchGroup_->setMaximumHeight(on ? QWIDGETSIZE_MAX : 24);  // collapsed: just the title, like the original
    tbMove_->setVisible((currentTab_ == 1 && on) || currentTab_ == 0);
}

// Modern layout: the tabs show how many files / planned actions there are.
void MainWindow::updateTabCounts() {
    if (!newUi_) return;
    const int n[] = {int(files_.size()), batchView_->topLevelItemCount()};
    const char* names[] = {"Files", "Actions"};
    for (int i = 0; i < 2; ++i) aTab_[i]->setText(n[i] > 0 ? tr_(names[i]) + u" (%1)"_s.arg(n[i]) : tr_(names[i]));
}

void MainWindow::setStatus() {
    countLabel_->setText(tr_("%1 files").arg(files_.size()));
    updateTabCounts();
    statusLabel_->clear();
    progress_->setValue(0);
    progress_->setTextVisible(false);
}

void MainWindow::setStatus(int percent, const QString& text) {
    if (percent >= 0) {
        progress_->setValue(percent);
        progress_->setTextVisible(percent > 0);
    }
    if (!text.isEmpty()) statusLabel_->setText(text);
}

QTreeWidget* MainWindow::currentList() const {
    return currentTab_ == 0 ? static_cast<QTreeWidget*>(fileView_) : currentTab_ == 1 ? static_cast<QTreeWidget*>(batchView_) : nullptr;
}

// --- files ------------------------------------------------------------------------------------------------------
File* MainWindow::fileOf(QTreeWidgetItem* item) const {
    return item ? reinterpret_cast<File*>(item->data(0, PtrRole).value<quintptr>()) : nullptr;
}

void MainWindow::updateItem(QTreeWidgetItem* item) {
    File* f = fileOf(item);
    if (!f) return;
    static QFileIconProvider provider;
    static QHash<QString, QIcon> iconCache;
    item->setText(ColFolder, f->fileFolder());
    item->setText(ColFileName, f->fileName());
    item->setText(ColFullPath, f->filePath());
    item->setText(ColExt, extractFileExt(f->fileName()));
    item->setText(ColPreview, f->preview);
    const bool hide = aChangedOnly_->isChecked() && !f->preview.isEmpty() && f->preview == f->fileName();
    item->setHidden(hide);
    if (hide) item->setSelected(false);  // a hidden file must never be hit by "Remove sel." or the move buttons
    auto visible = [this](int c) { return !fileView_->isColumnHidden(c); };
    if (visible(ColSize)) {
        const qint64 sz = f->isFolder() ? -1 : f->size();
        item->setText(ColSize, sz < 0 ? QString() : QLocale().toString(sz));
        item->setData(ColSize, SortRole, sz);
        item->setTextAlignment(ColSize, Qt::AlignRight | Qt::AlignVCenter);
    }
    auto date = [&](int c, const QDateTime& d) {
        item->setText(c, d.isValid() ? QLocale().toString(d, QLocale::ShortFormat) : QString());
        item->setData(c, SortRole, d.isValid() ? d.toMSecsSinceEpoch() : 0);
    };
    if (visible(ColCreated)) date(ColCreated, f->createdDate());
    if (visible(ColModified)) date(ColModified, f->modifiedDate());
    if (appOptions().fileIcons) {
        QIcon ic;
        if (f->isFolder()) {
            ic = provider.icon(QFileIconProvider::Folder);
        } else {
            QString ext = extractFileExt(f->fileName()).toLower();
            const QString key = (ext == u".exe" || ext == u".ico" || ext == u".lnk" || ext == u".pif") ? f->filePath().toLower() : ext;
            if (!iconCache.contains(key)) iconCache.insert(key, provider.icon(QFileInfo(f->filePath())));
            ic = iconCache.value(key);
        }
        item->setIcon(ColFolder, ic);
    } else {
        item->setIcon(ColFolder, QIcon());
    }
    if (f->isFolder()) {
        const QBrush b(folderColor(palette()));
        for (int c = 0; c < ColCount; ++c) item->setBackground(c, b);
    }
}

void MainWindow::updateAllItems() {
    fileView_->setUpdatesEnabled(false);
    for (int i = 0; i < fileView_->topLevelItemCount(); ++i) updateItem(fileView_->topLevelItem(i));
    fileView_->setUpdatesEnabled(true);
}

void MainWindow::addFile(const QString& path) {
    File& f = files_.add(path);
    auto* item = new FileItem;
    item->setData(0, PtrRole, quintptr(&f));
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
    fileView_->addTopLevelItem(item);
    rows_.insert(quintptr(&f), item);
    updateItem(item);
}

void MainWindow::addFolderContents(const QString& dir, const AddChoice& c) {
    QDir d(dir);
    const auto entries = d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::Name | QDir::DirsFirst);
    for (const QFileInfo& fi : entries) {
        const QString p = native(fi.absoluteFilePath());
        if (fi.isDir()) {
            if (!c.recursive) continue;
            if (c.folders) addFile(p);
            addFolderContents(p, c);
        } else if (c.files) {
            addFile(p);
        }
    }
}

// Port of TMainForm.AddFiles: with no choice, every path is added as is; otherwise folders are expanded.
void MainWindow::addFiles(const QStringList& paths, const AddChoice* choice) {
    if (worker_) return;  // the list belongs to the worker thread during a run
    setStatus(0, tr_("Adding files"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    fileView_->setUpdatesEnabled(false);
    for (const QString& p : paths) {
        const QFileInfo fi(p);
        if (choice && fi.isDir()) {
            if (choice->folders) addFile(native(p));
            addFolderContents(p, *choice);
        } else if (!choice || (choice->files && fi.isFile())) {
            addFile(native(p));
        }
    }
    fileView_->setUpdatesEnabled(true);
    QApplication::restoreOverrideCursor();
    setStatus();
    updateSamplePreview();
}

// Paths sent by the Explorer context menu: like a drop with the saved options, but without the question.
void MainWindow::addShellPaths(const QStringList& paths) {
    if (worker_ || paths.isEmpty()) return;
    const AddChoice c{true, appOptions().addFolders, appOptions().recursive};  // selected files are always added
    addFiles(paths, &c);
    showTab(0);
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
}

void MainWindow::filesDropped(const QStringList& paths) {
    if (worker_ || paths.isEmpty()) return;
    AddChoice c{appOptions().addFiles, appOptions().addFolders, appOptions().recursive};
    if (!appOptions().dragNoAsk) {
        DragDropDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;
        c = dlg.choice();
        appOptions().addFiles = c.files;
        appOptions().addFolders = c.folders;
        appOptions().recursive = c.recursive;
        appOptions().dragNoAsk = dlg.dontAskAgain();
        appOptions().save();
    }
    addFiles(paths, &c);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent* e) {
    QStringList paths;
    for (const QUrl& u : e->mimeData()->urls())
        if (u.isLocalFile()) paths << native(u.toLocalFile());
    filesDropped(paths);
}

void MainWindow::renumberFiles() {
    for (int i = 0; i < fileView_->topLevelItemCount(); ++i)
        if (File* f = fileOf(fileView_->topLevelItem(i))) f->position = i;
    files_.sort();
}

void MainWindow::removeSelectedFiles() {
    if (worker_) return;
    const auto sel = fileView_->selectedItems();
    for (QTreeWidgetItem* it : sel) {
        File* f = fileOf(it);
        rows_.remove(quintptr(f));
        files_.remove(f);
        delete it;
    }
    setStatus();
    updateSamplePreview();
}

void MainWindow::removeAllFiles() {
    if (worker_) return;
    fileView_->clear();
    rows_.clear();
    files_.clear();
    setStatus();
    updateSamplePreview();
}

void MainWindow::removeDeadFiles() {
    if (worker_) return;
    for (int i = fileView_->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem* it = fileView_->topLevelItem(i);
        File* f = fileOf(it);
        if (f && !f->exists()) {
            rows_.remove(quintptr(f));
            files_.remove(f);
            delete it;
        }
    }
    setStatus();
    updateSamplePreview();
}

void MainWindow::fileHeaderMenu(const QPoint& pos) {
    if (worker_) return;
    QMenu m;
    for (int c = 0; c < ColCount; ++c) {
        QAction* a = m.addAction(fileView_->headerItem()->text(c));
        a->setCheckable(true);
        a->setChecked(!fileView_->isColumnHidden(c));
        connect(a, &QAction::toggled, this, [this, c](bool on) {
            if (!on) {
                int shown = 0;
                for (int k = 0; k < ColCount; ++k) shown += !fileView_->isColumnHidden(k);
                if (shown <= 1) return;  // keep at least one column
            }
            fileView_->setColumnHidden(c, !on);
            if (on) updateAllItems();
        });
    }
    if (fileView_->header()->isSortIndicatorShown()) {
        m.addSeparator();
        m.addAction(aUnsort_);
    }
    m.exec(fileView_->header()->mapToGlobal(pos));
}

void MainWindow::sortFilesBy(int column) {
    if (worker_) return;
    // Own state: Qt's header may already have toggled its indicator by the time sectionClicked fires.
    auto* h = fileView_->header();
    sortOrder_ = (sortColumn_ == column && sortOrder_ == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    sortColumn_ = column;
    h->setSortIndicatorShown(true);
    h->setSortIndicator(column, sortOrder_);
    fileView_->sortItems(column, sortOrder_);
}

void MainWindow::unsortFiles() {
    if (worker_) return;
    sortColumn_ = -1;
    fileView_->header()->setSortIndicatorShown(false);
}

// --- batch -------------------------------------------------------------------------------------------------------
void MainWindow::addBatchItem(Action* a) {
    auto* item = new QTreeWidgetItem;
    item->setData(0, PtrRole, quintptr(a));
    item->setText(0, QString::number(batchView_->topLevelItemCount() + 1));
    item->setText(1, a->description());
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
    batchView_->addTopLevelItem(item);
}

void MainWindow::addToBatch() {
    if (worker_) return;
    auto a = panel_->makeAction(true);
    if (!a) return;
    Action* raw = a.get();
    batch_.add(std::move(a));
    addBatchItem(raw);
    if (!batchGroup_->isChecked()) batchGroup_->setChecked(true);
}

void MainWindow::renumberBatchView() {
    for (int i = 0; i < batchView_->topLevelItemCount(); ++i) batchView_->topLevelItem(i)->setText(0, QString::number(i + 1));
}

void MainWindow::renumberActions() {
    for (int i = 0; i < batchView_->topLevelItemCount(); ++i)
        reinterpret_cast<Action*>(batchView_->topLevelItem(i)->data(0, PtrRole).value<quintptr>())->position = i;
    batch_.sort();
}

void MainWindow::editBatchItem(QTreeWidgetItem* item) {
    if (worker_ || !item) return;
    if (const Action* a = reinterpret_cast<const Action*>(item->data(0, PtrRole).value<quintptr>())) {
        panel_->setFromAction(*a);
        batchView_->setCurrentItem(item);
        editIndex_ = QPersistentModelIndex(batchView_->indexFromItem(item));
        editLabel_->setText(tr_("Modification of action %1").arg(item->text(0)));
        editBar_->show();
        setStatus(-1, tr_("Settings loaded: change them, then validate the modification"));
    }
}

void MainWindow::applyEditorTo(QTreeWidgetItem* item) {
    auto a = panel_->makeAction(true);
    if (!a || !item) return;
    renumberActions();  // view order == list order from here on
    Action* raw = a.get();
    batch_.replace(size_t(batchView_->indexOfTopLevelItem(item)), std::move(a));
    item->setData(0, PtrRole, quintptr(raw));
    item->setText(1, raw->description());
    updateSamplePreview();
}

void MainWindow::updateBatchItem() {  // toolbar "Modify": the selected action
    if (worker_) return;
    const auto sel = batchView_->selectedItems();
    if (sel.size() != 1) return;
    applyEditorTo(sel.first());
    endEdit();
}

void MainWindow::validateEdit() {  // editor zone "Validate the modification": the action being edited
    if (worker_) return;
    if (editIndex_.isValid()) applyEditorTo(batchView_->topLevelItem(editIndex_.row()));
    endEdit();
}

void MainWindow::endEdit() {
    editIndex_ = QPersistentModelIndex();
    if (editBar_) editBar_->hide();
}

void MainWindow::removeSelectedActions() {
    if (worker_) return;
    renumberActions();  // view order == list order from here on
    QList<int> rows;
    for (auto* it : batchView_->selectedItems()) rows << batchView_->indexOfTopLevelItem(it);
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) {
        batch_.remove(size_t(r));
        delete batchView_->takeTopLevelItem(r);
    }
    renumberBatchView();
}

void MainWindow::clearBatch() {
    if (worker_) return;
    batchView_->clear();
    batch_.clear();
}

void MainWindow::openBatch() {
    QSettings s = appSettings();
    const QString p = QFileDialog::getOpenFileName(this, tr_("Open"), s.value(u"folders/batch"_s).toString(),
                                                   tr_("Ant Renamer batch files (*.arb *.xml)"));
    if (p.isEmpty()) return;
    s.setValue(u"folders/batch"_s, QFileInfo(p).absolutePath());
    openBatchFile(p);
}

void MainWindow::openBatchFile(const QString& path) {
    if (worker_) return;
    QSettings s = appSettings();
    if (!batch_.load(path)) {
        QMessageBox::critical(this, windowTitle(), tr_("Could not read %1").arg(native(path)));
        return;
    }
    QStringList mru = s.value(u"mru/batch"_s).toStringList();
    mru.removeAll(path);
    mru.prepend(path);
    while (mru.size() > 9) mru.removeLast();
    s.setValue(u"mru/batch"_s, mru);
    batchView_->clear();
    for (size_t i = 0; i < batch_.size(); ++i) addBatchItem(&batch_.at(i));
    if (batch_.size() > 0 && !batchGroup_->isChecked()) batchGroup_->setChecked(true);
}

void MainWindow::saveBatch() {
    if (worker_) return;  // saving renumbers (re-sorts) the batch the worker is walking
    QSettings s = appSettings();
    QString p = QFileDialog::getSaveFileName(this, tr_("Save"), s.value(u"folders/batch"_s).toString(),
                                             tr_("Ant Renamer batch files (*.arb)"));
    if (p.isEmpty()) return;
    if (QFileInfo(p).suffix().isEmpty()) p += u".arb"_s;
    s.setValue(u"folders/batch"_s, QFileInfo(p).absolutePath());
    renumberActions();
    if (!batch_.save(p)) QMessageBox::critical(this, windowTitle(), tr_("Could not write %1").arg(native(p)));
}

void MainWindow::updateRecentMenu() {
    recentMenu_->clear();
    const QStringList mru = appSettings().value(u"mru/batch"_s).toStringList();
    if (mru.isEmpty()) {
        recentMenu_->addAction(tr_("(no recent file)"))->setEnabled(false);
        return;
    }
    for (const QString& f : mru) recentMenu_->addAction(native(f), this, [this, f] { openBatchFile(f); });
}

// --- list helpers -----------------------------------------------------------------------------------------------
// mode: 0 up, 1 down, 2 top, 3 bottom. Works on the files list and on the batch list.
void MainWindow::moveItems(int mode) {
    QTreeWidget* t = currentList();
    if (!t || worker_) return;
    QList<QTreeWidgetItem*> sel = t->selectedItems();
    if (sel.isEmpty()) return;
    std::sort(sel.begin(), sel.end(), [t](auto* a, auto* b) { return t->indexOfTopLevelItem(a) < t->indexOfTopLevelItem(b); });
    const int n = t->topLevelItemCount();
    QSignalBlocker block(t);
    switch (mode) {
    case 0: {
        int limit = 0;
        for (auto* it : sel) {
            const int r = t->indexOfTopLevelItem(it);
            if (r == limit) { ++limit; continue; }
            t->insertTopLevelItem(r - 1, t->takeTopLevelItem(r));
        }
        break;
    }
    case 1: {
        int limit = n - 1;
        for (int i = int(sel.size()) - 1; i >= 0; --i) {
            const int r = t->indexOfTopLevelItem(sel[i]);
            if (r == limit) { --limit; continue; }
            t->insertTopLevelItem(r + 1, t->takeTopLevelItem(r));
        }
        break;
    }
    case 2:
        for (int i = 0; i < sel.size(); ++i) t->insertTopLevelItem(i, t->takeTopLevelItem(t->indexOfTopLevelItem(sel[i])));
        break;
    case 3:
        for (auto* it : sel) t->addTopLevelItem(t->takeTopLevelItem(t->indexOfTopLevelItem(it)));
        break;
    }
    for (auto* it : sel) it->setSelected(true);
    if (t == batchView_) renumberBatchView();
    else unsortFiles();
    if (!sel.isEmpty()) t->scrollToItem(sel.first());
}

// --- sample preview (one file, current action) ---------------------------------------------------------------------
void MainWindow::updateSamplePreview() {
    if (!isVisible() || worker_ || !previewGroup_->isChecked()) return;
    previewOld_->clear();
    previewNew_->clear();
    QTreeWidgetItem* item = fileView_->selectedItems().isEmpty() ? fileView_->topLevelItem(0) : fileView_->selectedItems().first();
    File* f = fileOf(item);
    auto action = panel_->makeAction(false);
    if (!f || !action) return;
    stopFlag() = false;
    FileList one;
    File& copy = one.add(f->filePath());
    action->perform(one, Op::Preview);
    previewOld_->setText(copy.fileName());
    previewNew_->setText(copy.preview);
}

// --- running ---------------------------------------------------------------------------------------------------------
void MainWindow::run(Op op) {
    if (worker_) {
        QMessageBox::information(this, windowTitle(), tr_("Application is busy: Renaming operation in progress."));
        return;
    }
    if (files_.size() == 0 || (batch_.size() == 0 && panel_->currentIndex() < 0 && op != Op::Revert)) {
        QMessageBox::warning(this, windowTitle(), tr_("Nothing to do: please select at least one file and an action to perform."));
        showTab(1);
        return;
    }
    const AppOptions& o = appOptions();
    if (op != Op::Preview) {
        if (o.startSwitch >= 0) showTab(o.startSwitch);
        if (o.startClearLog) logAction(0);
    } else {
        showTab(0);
        if (fileView_->isColumnHidden(ColPreview)) {
            fileView_->setColumnHidden(ColPreview, false);
            QSettings s = appSettings();
            if (s.value(u"hints/showPreview"_s, true).toBool()) {
                QMessageBox box(QMessageBox::Information, windowTitle(),
                                tr_("Preview column is now visible. If you want to hide it, click with the right mouse button on the column headers of the files list."),
                                QMessageBox::Ok, this);
                auto* again = new QCheckBox(tr_("Do not show this hint again"));
                box.setCheckBox(again);
                box.exec();
                if (again->isChecked()) s.setValue(u"hints/showPreview"_s, false);
            }
        }
    }
    runLogStart_ = logEntries_.size();
    addLog(op == Op::Rename ? tr_("Renaming process started") : op == Op::Preview ? tr_("Refreshing preview") : tr_("Revert to previous names"), State::Info);

    renumberFiles();
    renumberActions();
    aStop_->setEnabled(true);
    aGo_->setEnabled(false);
    setStatus(0, tr_("Renaming files"));

    if (op != Op::Revert && batch_.size() == 0) {  // empty batch: run the action configured on screen, then forget it
        auto a = panel_->makeAction(true);
        if (a) {
            Action* raw = a.get();
            batch_.add(std::move(a));
            addBatchItem(raw);
        }
        clearBatchAfter_ = true;
    } else {
        clearBatchAfter_ = op == Op::Rename && o.finishClearBatch;
    }
    if (op == Op::Rename)
        for (size_t i = 0; i < batch_.size(); ++i) addLog(u"%1. %2"_s.arg(i + 1).arg(batch_.at(i).description()), State::Info);

    stopFlag() = false;
    worker_ = new RenWorker(&batch_, &files_, op);
    connect(worker_, &RenWorker::progress, this, &MainWindow::onProgress, Qt::QueuedConnection);
    connect(worker_, &QThread::finished, this, &MainWindow::onFinished);
    setBusy(true);
    worker_->start();
}

void MainWindow::onProgress(const Snapshot& s) {
    const AppOptions& o = appOptions();
    if (!s.file) {
        addLog(s.description, State(s.state));
        return;
    }
    if (o.realtime) {
        if (QTreeWidgetItem* it = rows_.value(s.file)) {  // texts from the snapshot: the worker may already be editing this File again
            it->setText(ColFolder, s.folder);
            it->setText(ColFileName, s.name);
            it->setText(ColFullPath, s.folder + s.name);
            it->setText(ColExt, extractFileExt(s.name));
            it->setText(ColPreview, s.preview);
            const bool hide = aChangedOnly_->isChecked() && !s.preview.isEmpty() && s.preview == s.name;
            it->setHidden(hide);
            if (hide) it->setSelected(false);
        }
    }
    if (o.genLog) addLog(s.description, State(s.state));
    if (o.realtime) {
        const bool undo = worker_ && worker_->op() == Op::Revert;  // an undo makes one pass, whatever the batch holds
        const int div = int(files_.size() * (undo ? 1 : batch_.size()));
        if (div > 0) setStatus(int(((qint64(s.batchPos) * int(files_.size())) + s.index + 1) * 100 / div), {});
    }
}

void MainWindow::onFinished() {
    // the queued progress signals were posted before finished(); drain them first
    QCoreApplication::sendPostedEvents(this, QEvent::MetaCall);
    const Op op = worker_->op();
    worker_->deleteLater();
    worker_ = nullptr;
    setBusy(false);
    const AppOptions& o = appOptions();

    if (stopFlag()) addLog(tr_("Operation stopped by user"), State::Info);
    addLog(op == Op::Rename ? tr_("Renaming process finished") : op == Op::Preview ? tr_("Preview process finished") : tr_("Revert process finished"), State::Info);
    if (op != Op::Preview && o.finishSwitch >= 0) showTab(o.finishSwitch);
    if (clearBatchAfter_) clearBatch();
    if (o.finishClearFiles && op != Op::Preview) removeAllFiles();
    if (o.saveLog && op != Op::Preview && !o.saveLogFile.isEmpty()) {
        QFile f(o.saveLogFile);
        if (f.open(QIODevice::WriteOnly | (o.saveLogAppend ? QIODevice::Append : QIODevice::Truncate))) {
            if (f.size() == 0) f.write("\xEF\xBB\xBF");
            for (size_t i = qMin(runLogStart_, logEntries_.size()); i < logEntries_.size(); ++i) {  // this run only
                const LogEntry& e = logEntries_[i];
                const char* mark = e.state == State::NotRenamed ? " " : e.state == State::Ok ? "v" : e.state == State::Error ? "x" : nullptr;
                if (mark) f.write(QByteArray(mark) + '\t' + e.text.toUtf8() + "\r\n");
            }
        } else {
            addLog(tr_("Could not write %1").arg(native(o.saveLogFile)), State::Error);  // a log line, not a dialog: -g -x runs unattended
        }
    }
    aGo_->setEnabled(true);
    aStop_->setEnabled(false);
    setStatus();
    updateAllItems();
    updateSamplePreview();
    if (exitAfterRun_) close();
}

// While a run is in progress the lists, the batch and the options belong to the worker thread. The handlers also test
// worker_; disabling the actions makes it visible (and keeps their shortcuts from firing).
void MainWindow::setBusy(bool busy) {
    for (QAction* a : {aUndo_, aOptions_, aAddFiles_, aAddFolders_, aRemoveSel_, aRemoveAll_, aRemoveDead_, aPreview_, aChangedOnly_,
                       aBatchAdd_, aBatchRemoveSel_, aBatchClear_, aBatchOpen_, aBatchSave_, aUp_, aDown_, aTop_, aBottom_, aUnsort_})
        a->setEnabled(!busy);
    aBatchEdit_->setEnabled(!busy && newUi_ && batchView_->selectedItems().size() == 1);
}

// --- log ------------------------------------------------------------------------------------------------------------------
void MainWindow::addLog(const QString& text, State st) {
    if (text.isEmpty()) return;
    logEntries_.push_back({text, st});
    const bool shown = (st == State::Ok && logShowOk_) || (st == State::Error && logShowError_) ||
                       (st == State::NotRenamed && logShowNot_) || st == State::Info;
    if (!shown) return;
    auto* item = new QListWidgetItem(text, logView_);
    // dark red or navy on a dark background cannot be read: lighter shades there
    static const QColor light[] = {QColor(0, 0, 128), QColor(0, 128, 0), QColor(128, 0, 0), QColor(128, 128, 128)};
    static const QColor dark[] = {QColor(0x7a, 0xb4, 0xff), QColor(0x6f, 0xd0, 0x6f), QColor(0xff, 0x7b, 0x7b), QColor(0xaa, 0xaa, 0xaa)};
    const QColor* colors = isDarkPalette(logView_->palette()) ? dark : light;
    item->setForeground(colors[int(st)]);
    if (currentTab_ == 2 || singleScreen_) logView_->scrollToItem(item);
}

void MainWindow::rebuildLog() {
    const auto all = logEntries_;
    logEntries_.clear();
    logView_->clear();
    for (const LogEntry& e : all) addLog(e.text, e.state);
}

void MainWindow::logAction(int which) {
    if (which == 0) {
        logEntries_.clear();
        logView_->clear();
        return;
    }
    QStringList lines;
    for (int i = 0; i < logView_->count(); ++i) lines << logView_->item(i)->text();
    if (which == 2) {
        QApplication::clipboard()->setText(lines.join(u'\n'));
    } else {
        const QString p = QFileDialog::getSaveFileName(this, tr_("Save log"), {}, tr_("Text files (*.log *.txt)"));
        QFile f(p);
        if (p.isEmpty()) return;
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) f.write(lines.join(u"\r\n"_s).toUtf8());
        else QMessageBox::critical(this, windowTitle(), tr_("Could not write %1").arg(native(p)));
    }
}

// --- command line (same switches as the original) ---------------------------------------------------------------------------
// -b file.arb  load a batch      -g  start renaming      -x  exit when finished
// -a[fFr]      what the following paths add: f = files, F = folders, r = recursive; wildcards allowed
void MainWindow::processCommandLine(const QStringList& args) {
    bool go = false, exit = false;
    AddChoice add{false, false, false};
    bool haveAdd = false;
    const QString start = QDir::currentPath();
    for (int i = 0; i < args.size(); ++i) {
        const QString& a = args[i];
        if (a == u"--shell") {  // Explorer context menu: everything after it is a path
            addShellPaths(args.mid(i + 1));
            break;
        }
        if (a == u"-b" && i + 1 < args.size()) {
            openBatchFile(QDir(start).absoluteFilePath(args[++i]));
        } else if (a == u"-g") {
            go = true;
        } else if (a == u"-x") {
            exit = true;
        } else if (a.startsWith(u"-a")) {
            haveAdd = true;
            add = {a.contains(u'f'), a.contains(u'F'), a.contains(u'r')};
        } else if (!haveAdd || !add.any()) {
            const QString p = QDir(start).absoluteFilePath(a);
            if (a.endsWith(u".arb", Qt::CaseInsensitive)) openBatchFile(p);
            else if (QFileInfo(p).isDir()) {  // v2.07 compatibility
                const AddChoice legacy{true, false, true};
                addFiles({p}, &legacy);
            }
        } else {
            const QFileInfo fi(QDir(start).absoluteFilePath(a));
            const QString mask = fi.fileName() == u"*.*" ? u"*"_s : fi.fileName();  // "*.*" is "everything" for the user; for Qt it needs a dot
            const auto found = QDir(fi.absolutePath()).entryInfoList({mask}, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
            for (const QFileInfo& m : found) {
                if (m.isDir()) {
                    if (add.folders) addFile(native(m.absoluteFilePath()));
                    if (add.recursive) addFolderContents(m.absoluteFilePath(), add);
                } else if (add.files) {
                    addFile(native(m.absoluteFilePath()));
                }
            }
        }
    }
    setStatus();
    updateSamplePreview();
    if (go && files_.size() > 0) {
        exitAfterRun_ = exit;
        run(Op::Rename);
    } else if (exit) {
        QTimer::singleShot(0, this, [this] { close(); });  // from the event loop, so quitting always works
    }
}
