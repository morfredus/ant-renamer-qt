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

void MainWindow::run(Op op, int undoCount) {
    if (worker_) {
        QMessageBox::information(this, windowTitle(), tr_("Application is busy: Renaming operation in progress."));
        return;
    }
    if (op == Op::Undo ? journal_.size() == 0
                       : (files_.size() == 0 || (batch_.size() == 0 && panel_->currentIndex() < 0 && op != Op::Revert))) {
        QMessageBox::warning(this, windowTitle(),
                             op == Op::Undo ? tr_("Nothing to undo: no renaming pass was made in this session.")
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
    worker_ = new RenWorker(&batch_, &files_, op, undoCount);
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
        addLog(s.description, State(s.state));
    }
    if (o.realtime) {
        const bool undo = worker_ && worker_->op() == Op::Revert;  // an undo makes one pass, whatever the batch holds
        const int div = int(files_.size() * (undo ? 1 : batch_.size()));
        if (div > 0 && !(worker_ && worker_->op() == Op::Undo)) {
            setStatus(int(((qint64(s.batchPos) * int(files_.size())) + s.index + 1) * 100 / div), {});
        }
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

    if (stopFlag()) {
        addLog(tr_("Operation stopped by user"), State::Info);
    }
    addLog(op == Op::Rename    ? tr_("Renaming process finished")
           : op == Op::Preview ? tr_("Preview process finished")
                               : tr_("Revert process finished"),
           State::Info);
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
            if (f.size() == 0) {
                f.write("\xEF\xBB\xBF");
            }
            for (size_t i = qMin(runLogStart_, logEntries_.size()); i < logEntries_.size(); ++i) {  // this run only
                const LogEntry& e = logEntries_[i];
                const char* mark = e.state == State::NotRenamed ? " "
                                   : e.state == State::Ok       ? "v"
                                   : e.state == State::Error    ? "x"
                                                                : nullptr;
                if (mark) {
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
    updateUndoState();
    setStatus();
    updateAllItems();
    updateSamplePreview();
    if (exitAfterRun_) {
        close();
    }
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

// The passes of the session, newest first. Undo goes back linearly: choosing a pass undoes it and every pass after it.
void MainWindow::showHistory() {
    if (worker_ || journal_.size() == 0) {
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(tr_("Renaming history"));
    auto* lay = new QVBoxLayout(&dlg);
    lay->addWidget(
        new QLabel(tr_("Passes of this session, newest first. Passes are undone one after the other from the last one: "
                       "choosing a pass undoes it and all the passes made after it.")));
    dynamic_cast<QLabel*>(lay->itemAt(0)->widget())->setWordWrap(true);
    auto* list = new QListWidget;
    const auto& passes = journal_.passes();
    for (auto it = passes.rbegin(); it != passes.rend(); ++it) {
        list->addItem(tr_("#%1  %2  -  %3 move(s)  -  %4")
                          .arg(it->number)
                          .arg(it->when.toString(u"hh:mm:ss"_s))
                          .arg(it->moves.size())
                          .arg(it->summary));
    }
    list->setCurrentRow(0);
    lay->addWidget(list, 1);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close);
    auto* undoBtn = box->addButton(tr_("Undo up to this pass"), QDialogButtonBox::AcceptRole);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(list, &QListWidget::itemDoubleClicked, undoBtn, &QPushButton::click);
    lay->addWidget(box);
    dlg.resize(620, 360);
    if (dlg.exec() != QDialog::Accepted || list->currentRow() < 0) {
        return;
    }
    const int count = list->currentRow() + 1;
    if (count > 1 && QMessageBox::question(this, windowTitle(), tr_("Undo %1 passes?").arg(count)) != QMessageBox::Yes) {
        return;
    }
    run(Op::Undo, count);
}

// --- log ------------------------------------------------------------------------------------------------------------------
void MainWindow::addLog(const QString& text, State st) {
    if (text.isEmpty()) {
        return;
    }
    logEntries_.push_back({text, st});
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
        addLog(e.text, e.state);
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
        const QString p = QFileDialog::getSaveFileName(this, tr_("Save log"), {}, tr_("Text files (*.log *.txt)"));
        QFile f(p);
        if (p.isEmpty()) {
            return;
        }
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(lines.join(u"\r\n"_s).toUtf8());
        } else {
            QMessageBox::critical(this, windowTitle(), tr_("Could not write %1").arg(native(p)));
        }
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
    } else if (exit) {
        QTimer::singleShot(0, this, [this] { close(); });  // from the event loop, so quitting always works
    }
}
