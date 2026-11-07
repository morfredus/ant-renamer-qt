#include "mainwindow_internal.h"

// MainWindow: running the batch (worker thread, progress, undo), the log and the command line.

// --- running ---------------------------------------------------------------------------------------------------------
// Hidden or system files are rarely meant to be renamed: ask first. False when the user backs out.
bool MainWindow::confirmHiddenFiles() {
    int hidden = 0;
    if (aShowHidden_->isChecked()) {
        for (size_t i = 0; i < files_.size(); ++i) {
            hidden += files_.at(i).hidden;
        }
    }
    if (hidden && silent_) {  // nobody to ask: nothing is renamed
        quitSilently(2);
        return false;
    }
    return !hidden || QMessageBox::warning(this, windowTitle(),
                                           tr_("%1 hidden or system file(s) are in the list and will be renamed. Renaming them "
                                               "can break programs or the system. Continue?")
                                               .arg(hidden),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
}

// A preview needs the Preview column: show it, and say so once.
void MainWindow::showPreviewHint() {
    if (!fileView_->isColumnHidden(ColPreview)) {
        return;
    }
    fileView_->setColumnHidden(ColPreview, false);
    QSettings s = appSettings();
    if (!s.value(u"hints/showPreview"_s, true).toBool()) {
        return;
    }
    QMessageBox box(QMessageBox::Information, windowTitle(),
                    tr_("Preview column is now visible. If you want to hide it, click with the right mouse button on "
                        "the column headers of the files list."),
                    QMessageBox::Ok, this);
    auto* again = new QCheckBox(tr_("Do not show this hint again"));
    box.setCheckBox(again);
    box.exec();
    if (again->isChecked()) {
        s.setValue(u"hints/showPreview"_s, false);
    }
}

void MainWindow::run(Op op, int undoCount, int undoSession) {
    if (quitting_) {
        return;
    }
    if (worker_) {
        QMessageBox::information(this, windowTitle(), tr_("Application is busy: Renaming operation in progress."));
        return;
    }
    if (op == Op::Undo ? journal_.size() == 0
                       : (files_.size() == 0 || (batch_.size() == 0 && panel_->currentIndex() < 0 && op != Op::Revert))) {
        if (silent_) {
            quitSilently(2);
            return;
        }
        QMessageBox::warning(this, windowTitle(),
                             op == Op::Undo ? tr_("Nothing to undo: no renaming pass in the history.")
                                            : tr_("Nothing to do: please select at least one file and an action to perform."));
        if (op != Op::Undo) {
            showTab(1);
        }
        return;
    }
    if (op == Op::Rename && !confirmHiddenFiles()) {
        return;
    }
    const AppOptions& o = appOptions();
    if (op != Op::Preview) {
        if (o.startSwitch >= 0) {
            showTab(o.startSwitch);
        }
        if (o.startClearLog) {
            logAction(0);
        }
    } else {
        showTab(0);
        showPreviewHint();
    }
    runLogStart_ = logEntries_.size();
    runErrors_ = 0;
    runRenamed_ = 0;
    addLog(op == Op::Rename    ? tr_("Renaming process started")
           : op == Op::Preview ? tr_("Refreshing preview")
           : op == Op::Undo    ? tr_("Undoing the last renaming passes")
                               : tr_("Revert to previous names"),
           State::Info);

    renumberFiles();
    renumberActions();
    aStop_->setEnabled(true);
    aGo_->setEnabled(false);
    setStatus(0, tr_("Renaming files"));

    if (op != Op::Revert && op != Op::Undo &&
        batch_.size() == 0) {  // empty batch: run the action configured on screen, then forget it
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
    if (op == Op::Rename) {
        for (size_t i = 0; i < batch_.size(); ++i) {
            addLog(u"%1. %2"_s.arg(i + 1).arg(batch_.at(i).description()), State::Info);
        }
    }

    stopFlag() = false;
    worker_ = new RenWorker(&batch_, &files_, op, undoCount, undoSession);
    connect(worker_, &RenWorker::progress, this, &MainWindow::onProgress, Qt::QueuedConnection);
    connect(worker_, &QThread::finished, this, &MainWindow::onFinished);
    setBusy(true);
    worker_->start();
}

void MainWindow::onProgress(const Snapshot& s) {
    const AppOptions& o = appOptions();
    runErrors_ += State(s.state) == State::Error;
    runRenamed_ += State(s.state) == State::Ok && !s.from.isEmpty();  // from is set for a rename only
    if (!s.file) {
        addLog(s.description, State(s.state));
        return;
    }
    if (o.realtime) {
        if (QTreeWidgetItem* it =
                rows_.value(s.file)) {  // texts from the snapshot: the worker may already be editing this File again
            it->setText(ColFolder, s.folder);
            it->setText(ColFileName, s.name);
            it->setText(ColFullPath, s.folder + s.name);
            it->setText(ColExt, extractFileExt(s.name));
            it->setText(ColPreview, s.preview);
            const bool hide = aChangedOnly_->isChecked() && !s.preview.isEmpty() && s.preview == s.name;
            it->setHidden(hide);
            if (hide) {
                it->setSelected(false);
            }
        }
    }
    if (o.genLog) {
        // CSV row: a renamed file shows its old and new names, any other one the reason (the end of the log text)
        QStringList row;
        if (State(s.state) == State::Ok && !s.from.isEmpty()) {
            const bool sameFolder = extractFilePath(s.from) == s.folder;
            row = {extractFilePath(s.from), extractFileName(s.from), sameFolder ? s.name : s.folder + s.name, QString()};
        } else {
            const int k = s.description.lastIndexOf(u": "_s);
            row = {s.folder, s.name, QString(), State(s.state) == State::Ok ? tr_("Reverted") : s.description.mid(k < 0 ? 0 : k + 2)};
        }
        addLog(s.description, State(s.state), row);
    }
    if (o.realtime) {
        const bool undo = worker_ && worker_->op() == Op::Revert;  // an undo makes one pass, whatever the batch holds
        const int div = int(files_.size() * (undo ? 1 : batch_.size()));
        if (div > 0 && !(worker_ && worker_->op() == Op::Undo)) {
            setStatus(int(((qint64(s.batchPos) * int(files_.size())) + s.index + 1) * 100 / div), {});
        }
    }
}

// CSV of the log: Folder, Original name, New name, Reason (semicolon and quotes like the preview export, UTF-8 with a BOM:
// Excel and LibreOffice read the accents). Only the lines about files have a row.
static QByteArray logCsvLine(const QStringList& row) {
    QStringList q;
    for (const QString& v : row) {
        q << ren::csvField(v);
    }
    return (q.join(u';') + u"\r\n"_s).toUtf8();
}
static const QByteArray logCsvHeader = "\"Folder\";\"Original name\";\"New name\";\"Reason\"\r\n";

void MainWindow::onFinished() {
    // the queued progress signals were posted before finished(); drain them first
    QCoreApplication::sendPostedEvents(this, QEvent::MetaCall);
    const Op op = worker_->op();
    worker_->deleteLater();
    worker_ = nullptr;
    setBusy(false);
    const AppOptions& o = appOptions();

    if (stopFlag()) {
        addLog(tr_("Operation stopped by user"), State::Info);
    }
    addLog(op == Op::Rename    ? tr_("Renaming process finished")
           : op == Op::Preview ? tr_("Preview process finished")
                               : tr_("Revert process finished"),
           State::Info);
    const QString summary = op == Op::Rename ? tr_("Files renamed: %1, errors: %2").arg(runRenamed_).arg(runErrors_) : QString();
    if (!summary.isEmpty()) {
        addLog(summary, runErrors_ ? State::Error : State::Info);
    }
    if (op != Op::Preview && o.finishSwitch >= 0) {
        showTab(o.finishSwitch);
    }
    if (clearBatchAfter_) {
        clearBatch();
    }
    if (o.finishClearFiles && op != Op::Preview) {
        removeAllFiles();
    }
    if (o.saveLog && op != Op::Preview && !o.saveLogFile.isEmpty()) {
        QFile f(o.saveLogFile);
        if (f.open(QIODevice::WriteOnly | (o.saveLogAppend ? QIODevice::Append : QIODevice::Truncate))) {
            const bool csv = o.saveLogFile.endsWith(u".csv"_s, Qt::CaseInsensitive);
            if (f.size() == 0) {
                f.write("\xEF\xBB\xBF");  // BOM: Excel reads the accents
                if (csv) {
                    f.write(logCsvHeader);
                }
            }
            for (size_t i = qMin(runLogStart_, logEntries_.size()); i < logEntries_.size(); ++i) {  // this run only
                const LogEntry& e = logEntries_[i];
                const char* mark = e.state == State::NotRenamed ? " "
                                   : e.state == State::Ok       ? "v"
                                   : e.state == State::Error    ? "x"
                                                                : nullptr;
                if (csv) {
                    if (!e.row.isEmpty()) {
                        f.write(logCsvLine(e.row));
                    }
                } else if (mark) {
                    f.write(QByteArray(mark) + '\t' + e.text.toUtf8() + "\r\n");
                }
            }
        } else {
            addLog(tr_("Could not write %1").arg(native(o.saveLogFile)),
                   State::Error);  // a log line, not a dialog: -g -x runs unattended
        }
    }
    aGo_->setEnabled(true);
    aStop_->setEnabled(false);
    if (op != Op::Preview) {
        saveHistory();
    }
    updateUndoState();
    setStatus();
    statusLabel_->setText(summary);  // stays until the next run: the user sees at once that it is done
    updateAllItems();
    updateSamplePreview();
    if (exitAfterRun_ && silent_) {
        quitSilently(runErrors_ ? 1 : 0);
    } else if (exitAfterRun_) {
        close();
    }
}

// From the event loop, so that it also works before exec() has started. closeEvent() is not used: the window was never
// shown, its geometry must not overwrite the saved one.
void MainWindow::quitSilently(int code) {
    if (quitting_) {  // the first reason is the one that counts
        return;
    }
    quitting_ = true;
    QTimer::singleShot(0, qApp, [code] { QCoreApplication::exit(code); });
}

// While a run is in progress the lists, the batch and the options belong to the worker thread. The handlers also test
// worker_; disabling the actions makes it visible (and keeps their shortcuts from firing).
void MainWindow::setBusy(bool busy) {
    for (QAction* a :
         {aOptions_, aAddFiles_, aAddFolders_, aRemoveSel_, aRemoveAll_, aRemoveDead_, aPreview_, aChangedOnly_, aShowHidden_,
          aBatchAdd_, aBatchRemoveSel_, aBatchClear_, aBatchOpen_, aBatchSave_, aUp_, aDown_, aTop_, aBottom_, aUnsort_}) {
        a->setEnabled(!busy);
    }
    aBatchEdit_->setEnabled(!busy && newUi_ && batchView_->selectedItems().size() == 1);
    aUndo_->setEnabled(!busy && journal_.size() > 0);
    aHistory_->setEnabled(!busy && journal_.size() > 0);
}

void MainWindow::updateUndoState() {
    setBusy(worker_ != nullptr);
}

// The history is written after every run, so it survives a crash or a closed program. A failure is not worth a dialog.
void MainWindow::saveHistory() {
    const QString path = historyFile();
    if (journal_.size() == 0) {
        QFile::remove(path);
    } else if (QSaveFile f(path); f.open(QIODevice::WriteOnly)) {
        f.write(journal_.toJson());
        f.commit();
    }
}

// The passes of the history, newest first. Undo goes back linearly: choosing a pass undoes it and every pass after it.
void MainWindow::showHistory() {
    if (worker_ || journal_.size() == 0) {
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(tr_("Renaming history"));
    auto* lay = new QVBoxLayout(&dlg);
    lay->addWidget(
        new QLabel(tr_("Passes, newest first, by session (kept when the program is closed). Inside a session, passes are "
                       "undone one after the other from the last one: choosing a pass undoes it and the passes made after it "
                       "in the same session. The other sessions are not touched.")));
    dynamic_cast<QLabel*>(lay->itemAt(0)->widget())->setWordWrap(true);
    auto* list = new QListWidget;
    const auto& passes = journal_.passes();
    int shownSession = -1;
    for (auto it = passes.rbegin(); it != passes.rend(); ++it) {
        if (it->session != shownSession) {  // a heading for each session, dated by its first pass
            shownSession = it->session;
            const auto first = std::find_if(passes.begin(), passes.end(), [&](const ren::Pass& p) { return p.session == shownSession; });
            auto* head = new QListWidgetItem(tr_("Session of %1").arg(QLocale().toString(first->when, QLocale::ShortFormat)), list);
            head->setFlags(Qt::NoItemFlags);
            QFont f = head->font();
            f.setBold(true);
            head->setFont(f);
        }
        auto* item = new QListWidgetItem(tr_("#%1  %2  -  %3 move(s)  -  %4")
                                             .arg(it->number)
                                             .arg(QLocale().toString(it->when, QLocale::ShortFormat))
                                             .arg(it->moves.size())
                                             .arg(it->summary),
                                         list);
        item->setData(Qt::UserRole, it->session);
        item->setData(Qt::UserRole + 1, it->number);
    }
    list->setCurrentRow(1);  // row 0 is the heading of the newest session
    lay->addWidget(list, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close);
    auto* undoBtn = box->addButton(tr_("Undo up to this pass"), QDialogButtonBox::AcceptRole);
    // A pass that cannot be undone (file deleted or changed outside the program) blocks the ones before it: a way out.
    auto* clearBtn = box->addButton(tr_("Clear the history"), QDialogButtonBox::ResetRole);
    connect(clearBtn, &QPushButton::clicked, &dlg, [this, &dlg] {
        if (QMessageBox::question(&dlg, windowTitle(), tr_("Forget all the passes? They can no longer be undone.")) ==
            QMessageBox::Yes) {
            journal_.clear();
            saveHistory();
            updateUndoState();
            dlg.reject();
        }
    });
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(list, &QListWidget::itemDoubleClicked, undoBtn, &QPushButton::click);
    lay->addWidget(box);
    dlg.resize(620, 360);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const QListWidgetItem* chosen = list->currentItem();
    if (!chosen || !chosen->data(Qt::UserRole).isValid()) {
        return;
    }
    const int session = chosen->data(Qt::UserRole).toInt(), number = chosen->data(Qt::UserRole + 1).toInt();
    const int count = int(std::count_if(passes.begin(), passes.end(), [&](const ren::Pass& p) { return p.session == session && p.number >= number; }));
    if (count > 1 && QMessageBox::question(this, windowTitle(), tr_("Undo %1 passes?").arg(count)) != QMessageBox::Yes) {
        return;
    }
    run(Op::Undo, count, session);
}

// --- log ------------------------------------------------------------------------------------------------------------------
void MainWindow::addLog(const QString& text, State st, const QStringList& row) {
    if (text.isEmpty()) {
        return;
    }
    logEntries_.push_back({text, st, row});
    const bool shown = (st == State::Ok && logShowOk_) || (st == State::Error && logShowError_) ||
                       (st == State::NotRenamed && logShowNot_) || st == State::Info;
    if (!shown) {
        return;
    }
    auto* item = new QListWidgetItem(text, logView_);
    // dark red or navy on a dark background cannot be read: lighter shades there
    static const QColor light[] = {QColor(0, 0, 128), QColor(0, 128, 0), QColor(128, 0, 0), QColor(128, 128, 128)};
    static const QColor dark[] = {QColor(0x7a, 0xb4, 0xff), QColor(0x6f, 0xd0, 0x6f), QColor(0xff, 0x7b, 0x7b),
                                  QColor(0xaa, 0xaa, 0xaa)};
    const QColor* colors = isDarkPalette(logView_->palette()) ? dark : light;
    item->setForeground(colors[int(st)]);
    if (currentTab_ == 2 || singleScreen_) {
        logView_->scrollToItem(item);
    }
}

void MainWindow::rebuildLog() {
    const auto all = logEntries_;
    logEntries_.clear();
    logView_->clear();
    for (const LogEntry& e : all) {
        addLog(e.text, e.state, e.row);
    }
}

void MainWindow::logAction(int which) {
    if (which == 0) {
        logEntries_.clear();
        logView_->clear();
        return;
    }
    QStringList lines;
    for (int i = 0; i < logView_->count(); ++i) {
        lines << logView_->item(i)->text();
    }
    if (which == 2) {
        QApplication::clipboard()->setText(lines.join(u'\n'));
    } else {
        const QString csvFilter = tr_("CSV file (*.csv)");
        QString filter;
        QString p = QFileDialog::getSaveFileName(this, tr_("Save log"), {}, csvFilter + u";;"_s + tr_("Text files (*.log *.txt)"),
                                                 &filter);
        if (p.isEmpty()) {
            return;
        }
        p = withLogExtension(p, filter == csvFilter);
        const bool csv = p.endsWith(u".csv"_s, Qt::CaseInsensitive);
        QFile f(p);
        if (f.open(QIODevice::WriteOnly)) {  // not Text: the lines already end with CRLF, Text would make it CR CR LF
            if (csv) {  // the entries shown, with their status
                QByteArray out = "\xEF\xBB\xBF" + logCsvHeader;
                for (const LogEntry& e : logEntries_) {
                    if (!e.row.isEmpty() && ((e.state == State::Ok && logShowOk_) || (e.state == State::Error && logShowError_) ||
                        (e.state == State::NotRenamed && logShowNot_))) {
                        out += logCsvLine(e.row);
                    }
                }
                f.write(out);
            } else {
                f.write(lines.join(u"\r\n"_s).toUtf8());
            }
        } else {
            QMessageBox::critical(this, windowTitle(), tr_("Could not write %1").arg(native(p)));
        }
    }
}

// --- command line (same switches as the original) ---------------------------------------------------------------------------
// -b file.arb  load a batch      -g  start renaming      -x  exit when finished
// -q           silent: no window, no dialog; same as -g -x, the exit code tells how it went
// -a[fFr]      what the following paths add: f = files, F = folders, r = recursive; wildcards allowed
void MainWindow::processCommandLine(const QStringList& args) {
    silent_ = args.contains(u"-q"_s);
    bool go = silent_, exit = silent_;
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
        } else if (a == u"-q") {  // read before the loop (silent_)
            continue;
        } else if (a == u"-g") {
            go = true;
        } else if (a == u"-x") {
            exit = true;
        } else if (a.startsWith(u"-a")) {
            haveAdd = true;
            add = {a.contains(u'f'), a.contains(u'F'), a.contains(u'r')};
        } else if (!haveAdd || !add.any()) {
            const QString p = QDir(start).absoluteFilePath(a);
            if (a.endsWith(u".arb", Qt::CaseInsensitive)) {
                openBatchFile(p);
            } else if (QFileInfo(p).isDir()) {  // v2.07 compatibility
                const AddChoice legacy{true, false, true};
                addFiles({p}, &legacy);
            }
        } else {
            const QFileInfo fi(
                QDir::cleanPath(QDir(start).absoluteFilePath(a)));  // cleanPath drops a trailing slash: "d:\Data\" is "d:\Data"
            const QString mask =
                fi.fileName() == u"*.*" ? u"*"_s : fi.fileName();  // "*.*" is "everything" for the user; for Qt it needs a dot
            const auto found =
                QDir(fi.absolutePath()).entryInfoList({mask}, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
            for (const QFileInfo& m : found) {
                if (m.isDir()) {
                    if (add.folders) {
                        addFile(native(m.absoluteFilePath()));
                    }
                    if (add.recursive) {
                        addFolderContents(m.absoluteFilePath(), add);
                    }
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
    } else if (silent_) {
        quitSilently(2);  // no file to rename
    } else if (exit) {
        QTimer::singleShot(0, this, [this] { close(); });  // from the event loop, so quitting always works
    }
}
