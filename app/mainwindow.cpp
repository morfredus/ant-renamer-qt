#include "mainwindow.h"

#include "appsettings.h"
#include "icons.h"
#include "lngtranslator.h"
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
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QCollator>
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
            emit progress({quintptr(&f), f.description(), int(f.status()), batchPos, idx});
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
    setIconSet(appOptions().iconSet);
    appOptions().applyFont();
    createPages();
    createActions();
    createToolbars();
    createStatusBar();
    // Flat toolbar buttons with hover / pressed / checked feedback, like the original TBX toolbars.
    // rgba() so it works on light and dark themes.
    setStyleSheet(uR"(
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
    aOptions_ = makeAction(tr_("Options..."), tr_("Change program options"), TbOptions, K(Qt::Key_F10));
    aHelp_ = makeAction(tr_("Help"), tr_("Open help contents"), TbHelp, K(Qt::Key_F1));
    aAbout_ = makeAction(tr_("About..."), tr_("About this program"), TbAbout, K(Qt::SHIFT | Qt::Key_F1));
    aExit_ = makeAction(tr_("Exit"), tr_("Quit the program"), TbExit, K(Qt::ALT | Qt::Key_F4));

    const char* tabNames[] = {"Files", "Actions", "Log"};
    const char* tabHints[] = {"Display files", "Display actions", "Display log"};
    for (int i = 0; i < 3; ++i) {
        aTab_[i] = makeAction(tr_(tabNames[i]), tr_(tabHints[i]), TbTabFiles + i, K(Qt::CTRL | (Qt::Key_1 + i)));
        aTab_[i]->setCheckable(true);
        connect(aTab_[i], &QAction::triggered, this, [this, i] { showTab(i); });
    }

    aAddFiles_ = makeAction(tr_("Add files..."), tr_("Add files to the list"), TbAddFiles, K(Qt::Key_F2));
    aAddFolders_ = makeAction(tr_("Add folders..."), tr_("Add folders or folder contents to the list"), TbAddFolder, K(Qt::Key_F3));
    aRemoveSel_ = makeAction(tr_("Remove sel."), tr_("Remove selected files from the list"), TbRemoveSelected, K(Qt::Key_F6));
    aRemoveAll_ = makeAction(tr_("Remove all"), tr_("Remove all files from the list"), TbRemoveAll, K(Qt::Key_F7));
    aRemoveDead_ = makeAction(tr_("Remove dead"), tr_("Remove dead files from the list"), TbRemoveDead, K(Qt::Key_F8));
    aPreview_ = makeAction(tr_("Preview"), tr_("Refresh preview"), TbPreview, K(Qt::Key_F5));

    aBatchAdd_ = makeAction(tr_("Add to batch"), tr_("Add to batch"), TbBatchAdd, K(Qt::CTRL | Qt::Key_F2));
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
            for (int i = 0; i < t->topLevelItemCount(); ++i) t->topLevelItem(i)->setSelected(!t->topLevelItem(i)->isSelected());
    });
    connect(aUnsort_, &QAction::triggered, this, [this] { unsortFiles(); });
}

void MainWindow::createToolbars() {
    auto bar = [this](const QString& name) {
        auto* t = new QToolBar(name, this);
        t->setObjectName(name);
        t->setMovable(true);
        t->setIconSize(QSize(16, 16));
        t->setFloatable(false);
        return t;
    };
    // text beside the icon for the buttons the original labelled
    auto add = [](QToolBar* t, QAction* a, bool text) {
        t->addAction(a);
        if (auto* b = qobject_cast<QToolButton*>(t->widgetForAction(a)))
            b->setToolButtonStyle(text ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    };

    tbTabs_ = bar(u"Pages"_s);
    for (QAction* a : aTab_) add(tbTabs_, a, true);
    addToolBar(Qt::TopToolBarArea, tbTabs_);

    tbProgram_ = bar(u"Program"_s);
    add(tbProgram_, aGo_, true);
    add(tbProgram_, aStop_, true);
    add(tbProgram_, aUndo_, false);
    tbProgram_->addSeparator();
    add(tbProgram_, aOnTop_, false);
    add(tbProgram_, aOptions_, false);
    tbProgram_->addSeparator();
    add(tbProgram_, aHelp_, false);
    add(tbProgram_, aAbout_, false);
    tbProgram_->addSeparator();
    add(tbProgram_, aExit_, false);
    addToolBar(Qt::TopToolBarArea, tbProgram_);

    addToolBarBreak();

    tbFiles_ = bar(u"Files"_s);
    add(tbFiles_, aAddFiles_, true);
    add(tbFiles_, aAddFolders_, true);
    tbFiles_->addSeparator();
    add(tbFiles_, aRemoveSel_, false);
    add(tbFiles_, aRemoveAll_, true);
    add(tbFiles_, aRemoveDead_, false);
    tbFiles_->addSeparator();
    add(tbFiles_, aPreview_, false);
    addToolBar(Qt::TopToolBarArea, tbFiles_);

    tbActions_ = bar(u"Actions"_s);
    add(tbActions_, aBatchAdd_, false);
    add(tbActions_, aBatchRemoveSel_, false);
    tbActions_->addSeparator();
    add(tbActions_, aBatchClear_, false);
    recentMenu_ = new QMenu(this);
    auto* openBtn = new QToolButton;
    openBtn->setDefaultAction(aBatchOpen_);
    openBtn->setMenu(recentMenu_);
    openBtn->setPopupMode(QToolButton::MenuButtonPopup);
    tbActions_->addWidget(openBtn);
    connect(recentMenu_, &QMenu::aboutToShow, this, [this] { updateRecentMenu(); });
    add(tbActions_, aBatchSave_, false);
    addToolBar(Qt::TopToolBarArea, tbActions_);

    tbLog_ = bar(u"Log"_s);
    auto* filterMenu = new QMenu(this);
    filterMenu->addActions({aLogOk_, aLogError_, aLogNot_});
    auto* filterBtn = new QToolButton;
    filterBtn->setDefaultAction(aLogFilter_);
    filterBtn->setMenu(filterMenu);
    filterBtn->setPopupMode(QToolButton::InstantPopup);
    tbLog_->addWidget(filterBtn);
    tbLog_->addSeparator();
    add(tbLog_, aLogClear_, false);
    add(tbLog_, aLogSave_, false);
    add(tbLog_, aLogCopy_, false);
    addToolBar(Qt::TopToolBarArea, tbLog_);

    tbMove_ = bar(u"Move Items"_s);
    for (QAction* a : {aUp_, aDown_, aTop_, aBottom_}) add(tbMove_, a, false);
    addToolBar(Qt::TopToolBarArea, tbMove_);
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
    for (int c : {ColFullPath, ColExt, ColPreview, ColSize, ColCreated, ColModified}) fileView_->setColumnHidden(c, true);
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
    actionsSplit_ = new QSplitter(Qt::Vertical);
    actionsSplit_->setChildrenCollapsible(false);
    pl->addWidget(actionsSplit_);

    panel_ = new ActionPanel;
    actionsSplit_->addWidget(panel_);
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
    actionsSplit_->addWidget(previewGroup_);
    connect(previewGroup_, &QGroupBox::toggled, this, [this](bool on) {
        previewOld_->setVisible(on);
        previewNew_->setVisible(on);
        for (auto* l : previewGroup_->findChildren<QLabel*>()) l->setVisible(on);
        if (on) updateSamplePreview();
    });

    batchGroup_ = new QGroupBox(tr_("Batch contents (planned actions)"));
    batchGroup_->setCheckable(true);
    batchGroup_->setChecked(false);
    auto* bg = new QVBoxLayout(batchGroup_);
    batchView_ = new ItemTree(false);
    batchView_->setColumnCount(2);
    batchView_->setHeaderHidden(true);
    batchView_->setColumnWidth(0, 30);
    batchView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    batchView_->setContextMenuPolicy(Qt::CustomContextMenu);
    bg->addWidget(batchView_);
    actionsSplit_->addWidget(batchGroup_);
    actionsSplit_->setStretchFactor(0, 1);
    connect(batchGroup_, &QGroupBox::toggled, this, [this](bool on) { setBatchVisible(on); });
    connect(batchView_, &ItemTree::reordered, this, [this] { renumberBatchView(); });
    connect(batchView_, &QWidget::customContextMenuRequested, this, [this](const QPoint& p) {
        QMenu m;
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
    if (s.contains(u"window/geometry"_s)) restoreGeometry(s.value(u"window/geometry"_s).toByteArray());
    if (s.contains(u"window/filesHeader"_s)) fileView_->header()->restoreState(s.value(u"window/filesHeader"_s).toByteArray());
    if (s.contains(u"window/actionsSplit"_s)) actionsSplit_->restoreState(s.value(u"window/actionsSplit"_s).toByteArray());
    previewGroup_->setChecked(s.value(u"window/enableSample"_s, true).toBool());
    batchGroup_->setChecked(s.value(u"window/enableBatch"_s, false).toBool());
    setBatchVisible(batchGroup_->isChecked());
    aOnTop_->setChecked(s.value(u"window/onTop"_s, false).toBool());
    aLogOk_->setChecked(s.value(u"log/ok"_s, true).toBool());
    aLogError_->setChecked(s.value(u"log/error"_s, true).toBool());
    aLogNot_->setChecked(s.value(u"log/not"_s, true).toBool());
    if (s.contains(u"window/toolbars"_s)) restoreState(s.value(u"window/toolbars"_s).toByteArray());
    applyOptionsLive();
}

// Options that can change while the program runs (the others are read when used).
void MainWindow::applyOptionsLive() {
    const AppOptions& o = appOptions();
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
    s.setValue(u"window/geometry"_s, saveGeometry());
    s.setValue(u"window/toolbars"_s, saveState());
    s.setValue(u"window/filesHeader"_s, fileView_->header()->saveState());
    s.setValue(u"window/actionsSplit"_s, actionsSplit_->saveState());
    s.setValue(u"window/enableSample"_s, previewGroup_->isChecked());
    s.setValue(u"window/enableBatch"_s, batchGroup_->isChecked());
    s.setValue(u"window/onTop"_s, aOnTop_->isChecked());
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
    stack_->setCurrentIndex(tab);
    for (int i = 0; i < 3; ++i) aTab_[i]->setChecked(i == tab);
    tbActions_->setVisible(tab == 1);
    tbLog_->setVisible(tab == 2);
    tbFiles_->setVisible(true);  // the original keeps the files toolbar on every page
    tbMove_->setVisible((tab == 1 && batchGroup_->isChecked()) || tab == 0);
}

void MainWindow::setBatchVisible(bool on) {
    batchView_->setVisible(on);
    batchGroup_->setMaximumHeight(on ? QWIDGETSIZE_MAX : 24);  // collapsed: just the title, like the original
    tbMove_->setVisible((currentTab_ == 1 && on) || currentTab_ == 0);
}

void MainWindow::setStatus() {
    countLabel_->setText(tr_("%1 files").arg(files_.size()));
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
    // Own state: Qt's header may already have toggled its indicator by the time sectionClicked fires.
    auto* h = fileView_->header();
    sortOrder_ = (sortColumn_ == column && sortOrder_ == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    sortColumn_ = column;
    h->setSortIndicatorShown(true);
    h->setSortIndicator(column, sortOrder_);
    fileView_->sortItems(column, sortOrder_);
}

void MainWindow::unsortFiles() {
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
    worker_->start();
}

void MainWindow::onProgress(const Snapshot& s) {
    const AppOptions& o = appOptions();
    if (!s.file) {
        addLog(s.description, State(s.state));
        return;
    }
    if (o.realtime) {
        if (QTreeWidgetItem* it = rows_.value(s.file)) updateItem(it);
    }
    if (o.genLog) addLog(s.description, State(s.state));
    if (o.realtime) {
        const int div = int(files_.size() * batch_.size());
        if (div > 0) setStatus(int(((qint64(s.batchPos) * int(files_.size())) + s.index + 1) * 100 / div), {});
    }
}

void MainWindow::onFinished() {
    // the queued progress signals were posted before finished(); drain them first
    QCoreApplication::sendPostedEvents(this, QEvent::MetaCall);
    const Op op = worker_->op();
    worker_->deleteLater();
    worker_ = nullptr;
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
            for (const LogEntry& e : logEntries_) {
                const char* mark = e.state == State::NotRenamed ? " " : e.state == State::Ok ? "v" : e.state == State::Error ? "x" : nullptr;
                if (mark) f.write(QByteArray(mark) + '\t' + e.text.toUtf8() + "\r\n");
            }
        }
    }
    aGo_->setEnabled(true);
    aStop_->setEnabled(false);
    setStatus();
    updateAllItems();
    updateSamplePreview();
    if (exitAfterRun_) close();
}

// --- log ------------------------------------------------------------------------------------------------------------------
void MainWindow::addLog(const QString& text, State st) {
    if (text.isEmpty()) return;
    logEntries_.push_back({text, st});
    const bool shown = (st == State::Ok && logShowOk_) || (st == State::Error && logShowError_) ||
                       (st == State::NotRenamed && logShowNot_) || st == State::Info;
    if (!shown) return;
    auto* item = new QListWidgetItem(text, logView_);
    static const QColor colors[] = {QColor(0, 0, 128), QColor(0, 128, 0), QColor(128, 0, 0), QColor(128, 128, 128)};
    item->setForeground(colors[int(st)]);
    if (currentTab_ == 2) logView_->scrollToItem(item);
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
        if (!p.isEmpty() && f.open(QIODevice::WriteOnly | QIODevice::Text)) f.write(lines.join(u"\r\n"_s).toUtf8());
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
            const auto found = QDir(fi.absolutePath()).entryInfoList({fi.fileName()}, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
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
