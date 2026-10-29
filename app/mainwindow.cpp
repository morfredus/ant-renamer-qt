#include "mainwindow_internal.h"

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
    if (acceptUrls_ && e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        QTreeWidget::dragEnterEvent(e);
    }
}
void ItemTree::dragMoveEvent(QDragMoveEvent* e) {
    if (acceptUrls_ && e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        QTreeWidget::dragMoveEvent(e);
    }
}
void ItemTree::dropEvent(QDropEvent* e) {
    if (e->mimeData()->hasUrls()) {
        if (!acceptUrls_) {
            return;
        }
        QStringList paths;
        for (const QUrl& u : e->mimeData()->urls()) {
            if (u.isLocalFile()) {
                paths << native(u.toLocalFile());
            }
        }
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
        if (op_ == Op::Undo) {
            files_->undoPasses(undoCount_, [this](File& f) {
                emit progress({f.id(), f.description(), int(f.status()), 0, 0, QString(), f.fileFolder(), f.fileName(), f.preview});
            });
            return;
        }
        actions_->perform(*files_, op_, [this](File& f, int batchPos, int idx) {
            const QString from = op_ == Op::Rename && f.status() == State::Ok ? f.oldName() : QString();
            emit progress({f.id(), f.description(), int(f.status()), batchPos, idx, from, f.fileFolder(), f.fileName(), f.preview});
        });
    } catch (const std::exception& e) {
        emit progress(
            {0, QStringLiteral("Thread error caught: ") + QString::fromLocal8Bit(e.what()), int(State::Error), 0, 0, {}, {}, {}, {}});
    }
}

// --- construction -------------------------------------------------------------------------------------
QAction* MainWindow::makeAction(const QString& text, const QString& hint, int icon, const QKeySequence& key) {
    auto* a = new QAction(tbIcon(icon), text, this);
    a->setToolTip(hint);
    a->setData(icon);  // remembered so applyIcons() can switch icon set at run time
    if (!key.isEmpty()) {
        a->setShortcut(key);
    }
    a->setShortcutContext(Qt::WindowShortcut);
    addAction(a);  // keeps shortcuts alive when the toolbar button is hidden
    return a;
}

MainWindow::MainWindow() {
    qRegisterMetaType<Snapshot>("Snapshot");
    files_.setJournal(&journal_);
    setAcceptDrops(true);
    setMinimumSize(300, 300);
    appOptions().load();
    // keep the Move buttons on the list that has the focus (single screen: there is no current tab any more)
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* now) {
        for (int i = 0; i < 2 && now && singleScreen_; ++i) {
            if (panes_[i]->isAncestorOf(now)) {
                currentTab_ = i;
            }
        }
    });
    buildUi();
    aStop_->setEnabled(false);
    aUndo_->setEnabled(false);  // nothing to undo before the first pass
    aHistory_->setEnabled(false);
}

// Everything the user sees, from the options as they are now. The files, the batch, the journal and the log belong to the
// window and are not touched, so this can run again (see rebuildUi()).
void MainWindow::buildUi() {
    setWindowTitle(tr_("Ant Renamer Qt %1").arg(QApplication::applicationVersion()));
    resize(730, 500);

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
    }
    createStatusBar();
    // Flat toolbar buttons with hover / pressed / checked feedback, like the original TBX toolbars.
    // rgba() so it works on light and dark themes.
    if (newUi_) {
        setStyleSheet(uR"(
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
    } else {
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
    }
    loadSettings();
    setStatus();
    showTab(0);
}

// The language or the interface changed in the Options: save what the window remembers, throw the interface away and build
// it again (the texts are looked up when the widgets are made). The work in progress is kept.
void MainWindow::rebuildUi() {
    if (worker_) {
        return;
    }
    saveSettings();
    endEdit();
    const bool stayOnTop = aOnTop_->isChecked();
    setUpdatesEnabled(false);
    // the widgets first (their signals reach the actions), then the actions and the menus, which are children of the window
    if (QWidget* c = takeCentralWidget()) {
        delete c;
    }
    qDeleteAll(findChildren<QToolBar*>(Qt::FindDirectChildrenOnly));
    qDeleteAll(findChildren<QMenu*>(Qt::FindDirectChildrenOnly));
    qDeleteAll(findChildren<QAction*>(Qt::FindDirectChildrenOnly));
    for (QWidget* w : {static_cast<QWidget*>(progress_), static_cast<QWidget*>(countLabel_), static_cast<QWidget*>(statusLabel_)}) {
        statusBar()->removeWidget(w);
        delete w;
    }
    rows_.clear();
    sortColumn_ = -1;
    stack_ = nullptr;
    singleHost_ = nullptr;
    outer_ = inner_ = nullptr;
    editBar_ = nullptr;
    editLabel_ = nullptr;
    setStyleSheet(QString());

    buildUi();
    aOnTop_->setChecked(stayOnTop);

    // put the work back in the new views
    for (size_t i = 0; i < files_.size(); ++i) {
        File& f = files_.at(i);
        auto* item = new FileItem;
        item->setData(0, IdRole, f.id());
        item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
        fileView_->addTopLevelItem(item);
        rows_.insert(f.id(), item);
    }
    for (size_t i = 0; i < batch_.size(); ++i) {
        addBatchItem(&batch_.at(i));
    }
    rebuildLog();
    updateAllItems();
    updateUndoState();
    updateSamplePreview();
    setUpdatesEnabled(true);
    show();
}

void MainWindow::createActions() {
    using K = QKeySequence;
    aGo_ = makeAction(tr_("Go"), tr_("Start the renaming process"), TbGo, K(Qt::Key_F9));
    aStop_ = makeAction(tr_("Stop"), tr_("Stop the renaming process"), TbStop, K(Qt::Key_Escape));
    aUndo_ = makeAction(tr_("Undo"), tr_("Undo last renaming"), TbUndo, K(Qt::Key_F12));
    aHistory_ = makeAction(tr_("History"), tr_("Renaming passes of this session: go back to a given pass"), TbHistory,
                           K(Qt::SHIFT | Qt::Key_F12));
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
            if (singleScreen_) {
                setPaneVisible(i, on);  // the page buttons show / hide a pane
            } else {
                showTab(i);
            }
        });
    }

    aAddFiles_ = makeAction(tr_("Add files..."), tr_("Add files to the list"), TbAddFiles, K(Qt::Key_F2));
    aAddFolders_ =
        makeAction(tr_("Add folders..."), tr_("Add folders or folder contents to the list"), TbAddFolder, K(Qt::Key_F3));
    aRemoveSel_ = makeAction(tr_("Remove sel."), tr_("Remove selected files from the list"), TbRemoveSelected, K(Qt::Key_F6));
    aRemoveAll_ = makeAction(tr_("Remove all"), tr_("Remove all files from the list"), TbRemoveAll, K(Qt::Key_F7));
    aRemoveDead_ = makeAction(tr_("Remove dead"), tr_("Remove dead files from the list"), TbRemoveDead, K(Qt::Key_F8));
    aPreview_ = makeAction(tr_("Preview"), tr_("Refresh preview"), TbPreview, K(Qt::Key_F5));

    // view filter only: files hidden by "Changed only" stay in the list and are processed as usual
    aChangedOnly_ = makeAction(tr_("Changed only"), tr_("Show only the files whose name changes in the preview"), TbChangedOnly);
    aChangedOnly_->setCheckable(true);
    aShowHidden_ = makeAction(tr_("Hidden files"),
                              tr_("Show the hidden and system files of the list; hidden, they are not renamed"), TbShowHidden);
    aShowHidden_->setCheckable(true);
    connect(aShowHidden_, &QAction::toggled, this, [this] {
        ren::options().skipHidden = !aShowHidden_->isChecked();
        updateAllItems();
    });
    connect(aChangedOnly_, &QAction::toggled, this, [this] { updateAllItems(); });

    aBatchAdd_ = makeAction(tr_("Add to batch"), tr_("Add to batch"), TbBatchAdd, K(Qt::CTRL | Qt::Key_F2));
    aBatchEdit_ = makeAction(
        tr_("Modify"), tr_("Replace the selected action with the current settings (double-click an action to load its settings)"),
        TbBatchAdd);
    aBatchEdit_->setEnabled(false);
    connect(aBatchEdit_, &QAction::triggered, this, [this] { updateBatchItem(); });
    if (newUi_) {
        connect(batchView_, &QTreeWidget::itemSelectionChanged, this,
                [this] { aBatchEdit_->setEnabled(!worker_ && batchView_->selectedItems().size() == 1); });
        connect(batchView_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* it, int) { editBatchItem(it); });
    }
    aBatchRemoveSel_ = makeAction(tr_("Remove sel."), tr_("Remove selected actions from the batch"), TbBatchRemoveSel,
                                  K(Qt::SHIFT | Qt::Key_F6));
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
    aTop_ =
        makeAction(tr_("Top"), tr_("Move selected items to top of the list"), TbMoveTop, K(Qt::CTRL | Qt::SHIFT | Qt::Key_Up));
    aBottom_ = makeAction(tr_("Bottom"), tr_("Move selected items to bottom of the list"), TbMoveBottom,
                          K(Qt::CTRL | Qt::SHIFT | Qt::Key_Down));

    aSelAll_ = new QAction(tr_("Select all"), this);
    aSelNone_ = new QAction(tr_("Select none"), this);
    aSelInvert_ = new QAction(tr_("Invert selection"), this);
    aUnsort_ = new QAction(tr_("Unsorted items"), this);

    // wiring
    connect(aGo_, &QAction::triggered, this, [this] { run(Op::Rename); });
    connect(aStop_, &QAction::triggered, this, [] { stopFlag() = true; });
    connect(aUndo_, &QAction::triggered, this, [this] {
        if (appOptions().undoButton == 1) {
            showHistory();
        } else {
            run(Op::Undo, 1);  // F12 follows the choice made in the options
        }
    });
    connect(aHistory_, &QAction::triggered, this, &MainWindow::showHistory);
    connect(aPreview_, &QAction::triggered, this, [this] { run(Op::Preview); });
    connect(aOnTop_, &QAction::toggled, this, [this](bool on) {
        const bool vis = isVisible();
        setWindowFlag(Qt::WindowStaysOnTopHint, on);
        if (vis) {
            show();
        }
    });
    connect(aTheme_, &QAction::triggered, this, [this] {
        appOptions().theme = isDarkPalette(QGuiApplication::palette()) ? 1 : 2;
        appOptions().save();
        applyOptionsLive();
    });
    connect(aOptions_, &QAction::triggered, this, [this] {
        OptionsDialog dlg(this);
        const QString language = defaultLanguage();
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }
        const AppOptions& o = appOptions();
        const bool newLanguage = defaultLanguage() != language;
        if (newLanguage) {
            switchLanguage(defaultLanguage());
        }
        if (newLanguage || o.newUi != newUi_ || (o.newUi ? qBound(0, o.pageLayout, 2) : 0) != pageLayout_) {
            QTimer::singleShot(0, this, [this] { rebuildUi(); });  // not from inside the action that is being deleted
        } else {
            applyOptionsLive();
        }
    });
    connect(aHelp_, &QAction::triggered, this, [] { openHelp(u"index"_s); });
    connect(panel_, &ActionPanel::helpRequested, this, [](const QString& topic) { openHelp(topic); });
    connect(aAbout_, &QAction::triggered, this, [this] { AboutDialog(this).exec(); });
    connect(aExit_, &QAction::triggered, this, &QWidget::close);

    connect(aAddFiles_, &QAction::triggered, this, [this] {
        QSettings s = appSettings();
        const QStringList f = QFileDialog::getOpenFileNames(this, tr_("Select files"), s.value(u"folders/addFiles"_s).toString());
        if (f.isEmpty()) {
            return;
        }
        s.setValue(u"folders/addFiles"_s, QFileInfo(f.first()).absolutePath());
        QStringList n;
        for (const QString& p : f) {
            n << native(p);
        }
        addFiles(n, nullptr);
    });
    connect(aAddFolders_, &QAction::triggered, this, [this] {
        QSettings s = appSettings();
        AddFoldersDialog dlg(s.value(u"folders/addFolder"_s).toString(), this);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }
        const AddChoice c = dlg.choice();
        appOptions().addFiles = c.files;
        appOptions().addFolders = c.folders;
        appOptions().recursive = c.recursive;
        appOptions().addHidden = c.hidden;
        appOptions().save();
        if (!dlg.folders().isEmpty()) {
            s.setValue(u"folders/addFolder"_s, dlg.folders().first());
        }
        QStringList n;
        for (const QString& p : dlg.folders()) {
            n << native(p);
        }
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

    connect(aSelAll_, &QAction::triggered, this, [this] {
        if (auto* t = currentList()) {
            t->selectAll();
        }
    });
    connect(aSelNone_, &QAction::triggered, this, [this] {
        if (auto* t = currentList()) {
            t->clearSelection();
        }
    });
    connect(aSelInvert_, &QAction::triggered, this, [this] {
        if (auto* t = currentList()) {
            for (int i = 0; i < t->topLevelItemCount(); ++i) {
                if (!t->topLevelItem(i)->isHidden()) {
                    t->topLevelItem(i)->setSelected(!t->topLevelItem(i)->isSelected());
                }
            }
        }
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
                    if (auto* b = qobject_cast<QToolButton*>(t->widgetForAction(i.a))) {
                        b->setToolButtonStyle(i.kind == ActText ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
                    }
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
    Items files = {T(aAddFiles_),    T(aAddFolders_), sep, A(aRemoveSel_), T(aRemoveAll_), A(aRemoveDead_), sep, A(aPreview_),
                   A(aChangedOnly_), A(aShowHidden_)};
    Items actions = {newUi_ ? T(aBatchAdd_) : A(aBatchAdd_)};
    if (newUi_) {
        actions.push_back(T(aBatchEdit_));
    }
    append(actions, {A(aBatchRemoveSel_), sep, A(aBatchClear_), recent, A(aBatchSave_)});
    const Items log = {filter, sep, A(aLogClear_), A(aLogSave_), A(aLogCopy_)};

    if (!newUi_) {  // classic: bars on the window, movable; the move buttons have their own bar
        tbTabs_ = bar(u"Pages"_s, tabs);
        addToolBar(Qt::TopToolBarArea, tbTabs_);
        tbProgram_ = bar(u"Program"_s, {T(aGo_), T(aStop_), A(aUndo_), A(aHistory_), sep, A(aOnTop_), A(aTheme_), A(aOptions_),
                                        sep, A(aHelp_), A(aAbout_), sep, A(aExit_)});
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
    append(mainBar, {spacer, T(aGo_), T(aStop_), A(aUndo_), A(aHistory_), A(aOptions_), A(aHelp_), sep, A(aOnTop_), A(aTheme_),
                     A(aAbout_), A(aExit_)});
    tbProgram_ = bar(u"Main"_s, mainBar);
    if (auto* b = tbProgram_->widgetForAction(aGo_)) {
        b->setObjectName(u"goButton"_s);  // the one coloured button
    }
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
    for (auto sig : {&QAbstractItemModel::rowsInserted, &QAbstractItemModel::rowsRemoved}) {
        connect(batchView_->model(), sig, this, [this] {
            updateTabCounts();
            if (editBar_ && editBar_->isVisible() && !editIndex_.isValid()) {
                endEdit();
            }
        });
    }
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
    for (int c = 0; c < ColCount; ++c) {
        fileView_->setColumnWidth(c, widths[c]);
    }
    for (int c : {ColFullPath, ColExt, ColPreview, ColSize, ColCreated, ColModified}) {
        if (!(newUi_ && c == ColPreview)) {
            fileView_->setColumnHidden(c, true);  // modern layout: new name visible
        }
    }
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
        if (appOptions().launchFile) {
            if (File* f = fileOf(it)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(f->filePath()));
            }
        }
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
        m.addAction(tr_("Export preview..."), this, [this] { exportPreview(); });
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
    if (!newUi_) {
        actionsSplit_->addWidget(panel_);
    }
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
        connect(previewGroup_, &QGroupBox::toggled, this, [this](bool on) {
            if (!on) {
                previewGroup_->setChecked(true);
            }
        });  // always on
    } else {
        actionsSplit_->addWidget(previewGroup_);
    }
    connect(previewGroup_, &QGroupBox::toggled, this, [this](bool on) {
        previewOld_->setVisible(on);
        previewNew_->setVisible(on);
        for (auto* l : previewGroup_->findChildren<QLabel*>()) {
            l->setVisible(on);
        }
        if (on) {
            updateSamplePreview();
        }
    });

    batchGroup_ = new QGroupBox(tr_("Batch contents (planned actions)"));
    batchGroup_->setCheckable(!newUi_);  // modern layout: the plan is always visible
    if (newUi_) {
        batchGroup_->setMinimumWidth(170);  // the long title must not decide the width of the plan
    }
    if (!newUi_) {
        batchGroup_->setChecked(false);
    }
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
        if (newUi_) {
            m.addAction(aBatchEdit_);
        }
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
        for (int i = 0; i < 3; ++i) {
            paneVisible_[i] = mask & (1 << i);
        }
        arrangeSingle(singleMode_);  // applies the remembered sizes
        for (int i = 0; i < 3; ++i) {
            aTab_[i]->setChecked(paneVisible_[i]);
        }
    }
    if (s.contains(u"window/geometry"_s + sfx)) {
        restoreGeometry(s.value(u"window/geometry"_s + sfx).toByteArray());
    }
    if (s.contains(u"window/filesHeader"_s + sfx)) {
        fileView_->header()->restoreState(s.value(u"window/filesHeader"_s + sfx).toByteArray());
    }
    if (s.contains(u"window/actionsSplit"_s + sfx)) {
        actionsSplit_->restoreState(s.value(u"window/actionsSplit"_s + sfx).toByteArray());
    }
    if (!newUi_) {
        previewGroup_->setChecked(s.value(u"window/enableSample"_s, true).toBool());
        batchGroup_->setChecked(s.value(u"window/enableBatch"_s, false).toBool());
        setBatchVisible(batchGroup_->isChecked());
    }
    aOnTop_->setChecked(s.value(u"window/onTop"_s, false).toBool());
    aChangedOnly_->setChecked(s.value(u"view/changedOnly"_s, false).toBool());
    aShowHidden_->setChecked(s.value(u"view/showHidden"_s, false).toBool());
    aLogOk_->setChecked(s.value(u"log/ok"_s, true).toBool());
    aLogError_->setChecked(s.value(u"log/error"_s, true).toBool());
    aLogNot_->setChecked(s.value(u"log/not"_s, true).toBool());
    if (!newUi_ && s.contains(u"window/toolbars"_s)) {
        restoreState(s.value(u"window/toolbars"_s).toByteArray());
    }
    applyOptionsLive();
    QTimer::singleShot(2500, this, &MainWindow::startUpdateCheck);  // only if the user asked for it (Options > Display)
}

// Looks for a newer release once a day when the option is on. Never during a run or an unattended (-x) run.
void MainWindow::startUpdateCheck() {
    const AppOptions& o = appOptions();
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    if (runsInFlatpak() || !o.checkUpdates || o.lastUpdateCheck == today || exitAfterRun_ || worker_ ||
        qEnvironmentVariableIsSet("ANTREN_SHOT")) {
        return;
    }
    updater_ = new UpdateChecker(this);
    connect(updater_, &UpdateChecker::finished, this,
            [this, today](bool ok, const QString& version, const QString& url, const QString& error) {
                updater_->deleteLater();
                updater_ = nullptr;
                if (ok) {
                    appOptions().lastUpdateCheck = today;  // a failure is tried again at the next start
                    appOptions().save();
                }
                reportUpdate(this, ok, version, url, error, false);
            });
    updater_->check(o.checkPrereleases);
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
        for (QLabel* l : findChildren<QLabel*>()) {
            if (const QString html = l->text(); html.contains(u"<a "_s)) {  // (setText ignores the same text)
                l->clear();
                l->setText(html);
            }
        }
        rebuildLog();  // the colours of the lines depend on the theme
    }
    setIconSet(o.iconSet);
    for (QAction* a : findChildren<QAction*>()) {
        bool ok = false;
        const int idx = a->data().toInt(&ok);
        if (ok && a->data().isValid()) {
            a->setIcon(tbIcon(idx));
        }
    }
    for (QToolBar* t : findChildren<QToolBar*>()) {
        t->setIconSize(QSize(iconCell(), iconCell()));
    }
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
    s.setValue(u"view/showHidden"_s, aShowHidden_->isChecked());
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
    for (int i = 0; i < 3; ++i) {
        aTab_[i]->setChecked(i == tab);
    }
    if (newUi_) {
        return;  // modern layout: each page carries its own bar
    }
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
    arrangeSingle(singleMode_);     // moves the panes out of the stack first
    setCentralWidget(singleHost_);  // deletes the (now empty) stack
    stack_ = nullptr;
}

void MainWindow::arrangeSingle(int mode) {
    for (QWidget* p : panes_) {
        p->setParent(singleHost_);  // out of the old splitters, which are deleted below
    }
    delete outer_;  // also deletes inner_
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
        for (QWidget* p : panes_) {
            outer_->addWidget(p);
        }
    }
    singleHost_->layout()->addWidget(outer_);
    for (int i = 0; i < 3; ++i) {
        panes_[i]->setVisible(paneVisible_[i]);
    }
    if (mode == 2) {
        outer_->setStretchFactor(0, 1);
        outer_->setStretchFactor(1, 1);
        inner_->setStretchFactor(0, 3);
        inner_->setStretchFactor(1, 1);
        if (!stColsOuter_.isEmpty()) {
            outer_->restoreState(stColsOuter_);
        } else {
            outer_->setSizes({420, 880});
        }
        if (!stColsInner_.isEmpty()) {
            inner_->restoreState(stColsInner_);
        } else {
            inner_->setSizes({480, 180});
        }
    } else {
        outer_->setStretchFactor(0, 2);
        outer_->setStretchFactor(1, 3);
        outer_->setStretchFactor(2, 1);
        if (!stBands_.isEmpty()) {
            outer_->restoreState(stBands_);
        } else {
            outer_->setSizes({260, 320, 140});
        }
    }
}

void MainWindow::saveSingleState() {
    if (!outer_) {
        return;
    }
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
    if (!singleScreen_ || pageLayout_ != 2 || !outer_) {
        return;
    }
    const int want = singleMode_ == 2 ? (width() < 900 ? 1 : 2) : (width() >= 960 ? 2 : 1);
    if (want != singleMode_) {
        saveSingleState();
        arrangeSingle(want);
    }
}

void MainWindow::setBatchVisible(bool on) {
    if (newUi_) {
        return;  // the plan is always visible
    }
    batchView_->setVisible(on);
    batchGroup_->setMaximumHeight(on ? QWIDGETSIZE_MAX : 24);  // collapsed: just the title, like the original
    tbMove_->setVisible((currentTab_ == 1 && on) || currentTab_ == 0);
}

// Modern layout: the tabs show how many files / planned actions there are.
void MainWindow::updateTabCounts() {
    if (!newUi_) {
        return;
    }
    const int n[] = {int(files_.size()), batchView_->topLevelItemCount()};
    const char* names[] = {"Files", "Actions"};
    for (int i = 0; i < 2; ++i) {
        aTab_[i]->setText(n[i] > 0 ? tr_(names[i]) + u" (%1)"_s.arg(n[i]) : tr_(names[i]));
    }
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
    if (!text.isEmpty()) {
        statusLabel_->setText(text);
    }
}

QTreeWidget* MainWindow::currentList() const {
    return currentTab_ == 0   ? static_cast<QTreeWidget*>(fileView_)
           : currentTab_ == 1 ? static_cast<QTreeWidget*>(batchView_)
                              : nullptr;
}
