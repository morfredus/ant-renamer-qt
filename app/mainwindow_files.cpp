#include "mainwindow_internal.h"

// MainWindow: the files list (add, remove, sort, move, drop) and the preview of one file.

// --- files ------------------------------------------------------------------------------------------------------
Action* MainWindow::actionOf(QTreeWidgetItem* item) {
    return item ? batch_.byId(item->data(0, IdRole).toULongLong()) : nullptr;
}

File* MainWindow::fileOf(QTreeWidgetItem* item) const {
    return item ? files_.byId(item->data(0, IdRole).toULongLong()) : nullptr;
}

void MainWindow::updateItem(QTreeWidgetItem* item) {
    File* f = fileOf(item);
    if (!f) {
        return;
    }
    static QFileIconProvider provider;
    static QHash<QString, QIcon> iconCache;
    item->setText(ColFolder, f->fileFolder());
    item->setText(ColFileName, f->fileName());
    item->setText(ColFullPath, f->filePath());
    item->setText(ColExt, extractFileExt(f->fileName()));
    item->setText(ColPreview, f->preview);
    const bool hide = (aChangedOnly_->isChecked() && !f->preview.isEmpty() && f->preview == f->fileName()) ||
                      (f->hidden && !aShowHidden_->isChecked());
    item->setHidden(hide);
    if (hide) {
        item->setSelected(false);  // a hidden file must never be hit by "Remove sel." or the move buttons
    }
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
    if (visible(ColCreated)) {
        date(ColCreated, f->createdDate());
    }
    if (visible(ColModified)) {
        date(ColModified, f->modifiedDate());
    }
    if (appOptions().fileIcons) {
        QIcon ic;
        if (f->isFolder()) {
            ic = provider.icon(QFileIconProvider::Folder);
        } else {
            QString ext = extractFileExt(f->fileName()).toLower();
            const QString key =
                (ext == u".exe" || ext == u".ico" || ext == u".lnk" || ext == u".pif") ? f->filePath().toLower() : ext;
            if (!iconCache.contains(key)) {
                iconCache.insert(key, provider.icon(QFileInfo(f->filePath())));
            }
            ic = iconCache.value(key);
        }
        item->setIcon(ColFolder, ic);
    } else {
        item->setIcon(ColFolder, QIcon());
    }
    if (f->isFolder()) {
        const QBrush b(folderColor(palette()));
        for (int c = 0; c < ColCount; ++c) {
            item->setBackground(c, b);
        }
    }
}

void MainWindow::updateAllItems() {
    fileView_->setUpdatesEnabled(false);
    for (int i = 0; i < fileView_->topLevelItemCount(); ++i) {
        updateItem(fileView_->topLevelItem(i));
    }
    fileView_->setUpdatesEnabled(true);
}

void MainWindow::addFile(const QString& path) {
    File& f = files_.add(path);
    f.hidden = isHiddenOrSystem(QFileInfo(path));
    auto* item = new FileItem;
    item->setData(0, IdRole, f.id());
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
    fileView_->addTopLevelItem(item);
    rows_.insert(f.id(), item);
    updateItem(item);
}

// The files shown in the list (the "Changed only" and hidden-files filters apply), as a CSV or a list of new names.
void MainWindow::exportPreview() {
    std::vector<const File*> shown;
    for (int i = 0; i < fileView_->topLevelItemCount(); ++i) {
        QTreeWidgetItem* it = fileView_->topLevelItem(i);
        if (!it->isHidden()) {
            if (const File* f = fileOf(it)) {
                shown.push_back(f);
            }
        }
    }
    QSettings s = appSettings();
    QString filter;
    const QString csvFilter = tr_("CSV file, current and new names (*.csv)");
    const QString txtFilter = tr_("Text file, new names only (*.txt)");
    const QString path = QFileDialog::getSaveFileName(this, tr_("Export preview"), s.value(u"folders/exportPreview"_s).toString(),
                                                      csvFilter + u";;"_s + txtFilter, &filter);
    if (path.isEmpty()) {
        return;
    }
    s.setValue(u"folders/exportPreview"_s, path);
    const bool csv =
        path.endsWith(u".csv"_s, Qt::CaseInsensitive) || (filter == csvFilter && !path.endsWith(u".txt"_s, Qt::CaseInsensitive));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr_("Export preview"), tr_("Could not write %1").arg(native(path)));
        return;
    }
    if (csv) {
        f.write("\xEF\xBB\xBF");  // BOM: Excel reads the accents
    }
    f.write(ren::exportPreview(shown, csv).toUtf8());
}

void MainWindow::addFolderContents(const QString& dir, const AddChoice& c) {
    QDir d(dir);
    const auto entries =
        d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::Name | QDir::DirsFirst);
    for (const QFileInfo& fi : entries) {
        if (!c.hidden && isHiddenOrSystem(fi)) {
            continue;  // a hidden folder is not entered either
        }
        const QString p = native(fi.absoluteFilePath());
        if (fi.isDir()) {
            if (!c.recursive) {
                continue;
            }
            if (c.folders) {
                addFile(p);
            }
            addFolderContents(p, c);
        } else if (c.files) {
            addFile(p);
        }
    }
}

// Port of TMainForm.AddFiles: with no choice, every path is added as is; otherwise folders are expanded.
void MainWindow::addFiles(const QStringList& paths, const AddChoice* choice) {
    if (worker_) {
        return;  // the list belongs to the worker thread during a run
    }
    setStatus(0, tr_("Adding files"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    fileView_->setUpdatesEnabled(false);
    for (const QString& p : paths) {
        const QFileInfo fi(p);
        if (choice && fi.isDir()) {
            if (choice->folders) {
                addFile(native(p));
            }
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
    if (worker_ || paths.isEmpty()) {
        return;
    }
    const AddChoice c{true, appOptions().addFolders, appOptions().recursive,
                      appOptions().addHidden};  // selected files are always added
    addFiles(paths, &c);
    showTab(0);
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
}

// The question is asked once the drop is over: a modal window opened inside the drop handler stays behind the file
// manager, which is still waiting for the end of its drag.
void MainWindow::filesDropped(const QStringList& paths) {
    if (worker_ || paths.isEmpty()) {
        return;
    }
    QTimer::singleShot(0, this, [this, paths] { importDropped(paths); });
}

void MainWindow::importDropped(const QStringList& paths) {
    if (worker_) {
        return;
    }
    AddChoice c{appOptions().addFiles, appOptions().addFolders, appOptions().recursive, appOptions().addHidden};
    if (!appOptions().dragNoAsk) {
        bringToFront(this);
        DragDropDialog dlg(this);
        dlg.show();
        bringToFront(&dlg);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }
        c = dlg.choice();
        appOptions().addFiles = c.files;
        appOptions().addFolders = c.folders;
        appOptions().recursive = c.recursive;
        appOptions().addHidden = c.hidden;
        appOptions().dragNoAsk = dlg.dontAskAgain();
        appOptions().save();
    }
    addFiles(paths, &c);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}
void MainWindow::dropEvent(QDropEvent* e) {
    QStringList paths;
    for (const QUrl& u : e->mimeData()->urls()) {
        if (u.isLocalFile()) {
            paths << native(u.toLocalFile());
        }
    }
    filesDropped(paths);
}

void MainWindow::renumberFiles() {
    for (int i = 0; i < fileView_->topLevelItemCount(); ++i) {
        if (File* f = fileOf(fileView_->topLevelItem(i))) {
            f->position = i;
        }
    }
    files_.sort();
}

void MainWindow::removeSelectedFiles() {
    if (worker_) {
        return;
    }
    const auto sel = fileView_->selectedItems();
    for (QTreeWidgetItem* it : sel) {
        if (File* f = fileOf(it)) {
            rows_.remove(f->id());
            files_.remove(f);
        }
        delete it;
    }
    setStatus();
    updateSamplePreview();
}

void MainWindow::removeAllFiles() {
    if (worker_) {
        return;
    }
    fileView_->clear();
    rows_.clear();
    files_.clear();
    setStatus();
    updateSamplePreview();
}

void MainWindow::removeDeadFiles() {
    if (worker_) {
        return;
    }
    for (int i = fileView_->topLevelItemCount() - 1; i >= 0; --i) {
        QTreeWidgetItem* it = fileView_->topLevelItem(i);
        File* f = fileOf(it);
        if (f && !f->exists()) {
            rows_.remove(f->id());
            files_.remove(f);
            delete it;
        }
    }
    setStatus();
    updateSamplePreview();
}

void MainWindow::fileHeaderMenu(const QPoint& pos) {
    if (worker_) {
        return;
    }
    QMenu m;
    for (int c = 0; c < ColCount; ++c) {
        QAction* a = m.addAction(fileView_->headerItem()->text(c));
        a->setCheckable(true);
        a->setChecked(!fileView_->isColumnHidden(c));
        connect(a, &QAction::toggled, this, [this, c](bool on) {
            if (!on) {
                int shown = 0;
                for (int k = 0; k < ColCount; ++k) {
                    shown += !fileView_->isColumnHidden(k);
                }
                if (shown <= 1) {
                    return;  // keep at least one column
                }
            }
            fileView_->setColumnHidden(c, !on);
            if (on) {
                updateAllItems();
            }
        });
    }
    if (fileView_->header()->isSortIndicatorShown()) {
        m.addSeparator();
        m.addAction(aUnsort_);
    }
    m.exec(fileView_->header()->mapToGlobal(pos));
}

void MainWindow::sortFilesBy(int column) {
    if (worker_) {
        return;
    }
    // Own state: Qt's header may already have toggled its indicator by the time sectionClicked fires.
    auto* h = fileView_->header();
    sortOrder_ = (sortColumn_ == column && sortOrder_ == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    sortColumn_ = column;
    h->setSortIndicatorShown(true);
    h->setSortIndicator(column, sortOrder_);
    fileView_->sortItems(column, sortOrder_);
}

void MainWindow::unsortFiles() {
    if (worker_) {
        return;
    }
    sortColumn_ = -1;
    fileView_->header()->setSortIndicatorShown(false);
}

// --- list helpers -----------------------------------------------------------------------------------------------
// mode: 0 up, 1 down, 2 top, 3 bottom. Works on the files list and on the batch list.
void MainWindow::moveItems(int mode) {
    QTreeWidget* t = currentList();
    if (!t || worker_) {
        return;
    }
    QList<QTreeWidgetItem*> sel = t->selectedItems();
    if (sel.isEmpty()) {
        return;
    }
    std::sort(sel.begin(), sel.end(), [t](auto* a, auto* b) { return t->indexOfTopLevelItem(a) < t->indexOfTopLevelItem(b); });
    const int n = t->topLevelItemCount();
    QSignalBlocker block(t);
    switch (mode) {
        case 0: {
            int limit = 0;
            for (auto* it : sel) {
                const int r = t->indexOfTopLevelItem(it);
                if (r == limit) {
                    ++limit;
                    continue;
                }
                t->insertTopLevelItem(r - 1, t->takeTopLevelItem(r));
            }
            break;
        }
        case 1: {
            int limit = n - 1;
            for (int i = int(sel.size()) - 1; i >= 0; --i) {
                const int r = t->indexOfTopLevelItem(sel[i]);
                if (r == limit) {
                    --limit;
                    continue;
                }
                t->insertTopLevelItem(r + 1, t->takeTopLevelItem(r));
            }
            break;
        }
        case 2:
            for (int i = 0; i < sel.size(); ++i) {
                t->insertTopLevelItem(i, t->takeTopLevelItem(t->indexOfTopLevelItem(sel[i])));
            }
            break;
        case 3:
            for (auto* it : sel) {
                t->addTopLevelItem(t->takeTopLevelItem(t->indexOfTopLevelItem(it)));
            }
            break;
    }
    for (auto* it : sel) {
        it->setSelected(true);
    }
    if (t == batchView_) {
        renumberBatchView();
    } else {
        unsortFiles();
    }
    if (!sel.isEmpty()) {
        t->scrollToItem(sel.first());
    }
}

// --- sample preview (one file, current action) ---------------------------------------------------------------------
void MainWindow::updateSamplePreview() {
    if (!isVisible() || worker_ || !previewGroup_->isChecked()) {
        return;
    }
    previewOld_->clear();
    previewNew_->clear();
    QTreeWidgetItem* item =
        fileView_->selectedItems().isEmpty() ? fileView_->topLevelItem(0) : fileView_->selectedItems().first();
    File* f = fileOf(item);
    auto action = panel_->makeAction(false);
    if (!f || !action) {
        return;
    }
    stopFlag() = false;
    FileList one;
    File& copy = one.add(f->filePath());
    action->perform(one, Op::Preview);
    previewOld_->setText(copy.fileName());
    previewNew_->setText(copy.preview);
}
