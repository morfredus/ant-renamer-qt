#include "mainwindow_internal.h"

// MainWindow: the batch of actions (add, edit, remove, open and save).

// --- batch -------------------------------------------------------------------------------------------------------
void MainWindow::addBatchItem(Action* a) {
    auto* item = new QTreeWidgetItem;
    item->setData(0, IdRole, a->id());
    item->setText(0, QString::number(batchView_->topLevelItemCount() + 1));
    item->setText(1, a->description());
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
    batchView_->addTopLevelItem(item);
}

void MainWindow::addToBatch() {
    if (worker_) {
        return;
    }
    auto a = panel_->makeAction(true);
    if (!a) {
        return;
    }
    Action* raw = a.get();
    batch_.add(std::move(a));
    addBatchItem(raw);
    if (!batchGroup_->isChecked()) {
        batchGroup_->setChecked(true);
    }
}

void MainWindow::renumberBatchView() {
    for (int i = 0; i < batchView_->topLevelItemCount(); ++i) {
        batchView_->topLevelItem(i)->setText(0, QString::number(i + 1));
    }
}

void MainWindow::renumberActions() {
    for (int i = 0; i < batchView_->topLevelItemCount(); ++i) {
        if (Action* a = actionOf(batchView_->topLevelItem(i))) {
            a->position = i;
        }
    }
    batch_.sort();
}

void MainWindow::editBatchItem(QTreeWidgetItem* item) {
    if (worker_ || !item) {
        return;
    }
    if (const Action* a = actionOf(item)) {
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
    if (!a || !item) {
        return;
    }
    renumberActions();  // view order == list order from here on
    Action* raw = a.get();
    batch_.replace(size_t(batchView_->indexOfTopLevelItem(item)), std::move(a));
    item->setData(0, IdRole, raw->id());
    item->setText(1, raw->description());
    updateSamplePreview();
}

void MainWindow::updateBatchItem() {  // toolbar "Modify": the selected action
    if (worker_) {
        return;
    }
    const auto sel = batchView_->selectedItems();
    if (sel.size() != 1) {
        return;
    }
    applyEditorTo(sel.first());
    endEdit();
}

void MainWindow::validateEdit() {  // editor zone "Validate the modification": the action being edited
    if (worker_) {
        return;
    }
    if (editIndex_.isValid()) {
        applyEditorTo(batchView_->topLevelItem(editIndex_.row()));
    }
    endEdit();
}

void MainWindow::endEdit() {
    editIndex_ = QPersistentModelIndex();
    if (editBar_) {
        editBar_->hide();
    }
}

void MainWindow::removeSelectedActions() {
    if (worker_) {
        return;
    }
    renumberActions();  // view order == list order from here on
    QList<int> rows;
    for (auto* it : batchView_->selectedItems()) {
        rows << batchView_->indexOfTopLevelItem(it);
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) {
        batch_.remove(size_t(r));
        delete batchView_->takeTopLevelItem(r);
    }
    renumberBatchView();
}

void MainWindow::clearBatch() {
    if (worker_) {
        return;
    }
    batchView_->clear();
    batch_.clear();
}

void MainWindow::openBatch() {
    QSettings s = appSettings();
    const QString p = QFileDialog::getOpenFileName(this, tr_("Open"), s.value(u"folders/batch"_s).toString(),
                                                   tr_("Ant Renamer batch files (*.arb *.xml)"));
    if (p.isEmpty()) {
        return;
    }
    s.setValue(u"folders/batch"_s, QFileInfo(p).absolutePath());
    openBatchFile(p);
}

void MainWindow::openBatchFile(const QString& path) {
    if (worker_) {
        return;
    }
    QSettings s = appSettings();
    if (!batch_.load(path)) {
        QMessageBox::critical(this, windowTitle(), tr_("Could not read %1").arg(native(path)));
        return;
    }
    QStringList mru = s.value(u"mru/batch"_s).toStringList();
    mru.removeAll(path);
    mru.prepend(path);
    while (mru.size() > 9) {
        mru.removeLast();
    }
    s.setValue(u"mru/batch"_s, mru);
    batchView_->clear();
    for (size_t i = 0; i < batch_.size(); ++i) {
        addBatchItem(&batch_.at(i));
    }
    if (batch_.size() > 0 && !batchGroup_->isChecked()) {
        batchGroup_->setChecked(true);
    }
}

void MainWindow::saveBatch() {
    if (worker_) {
        return;  // saving renumbers (re-sorts) the batch the worker is walking
    }
    QSettings s = appSettings();
    QString p = QFileDialog::getSaveFileName(this, tr_("Save"), s.value(u"folders/batch"_s).toString(),
                                             tr_("Ant Renamer batch files (*.arb)"));
    if (p.isEmpty()) {
        return;
    }
    if (QFileInfo(p).suffix().isEmpty()) {
        p += u".arb"_s;
    }
    s.setValue(u"folders/batch"_s, QFileInfo(p).absolutePath());
    renumberActions();
    if (!batch_.save(p)) {
        QMessageBox::critical(this, windowTitle(), tr_("Could not write %1").arg(native(p)));
    }
}

void MainWindow::updateRecentMenu() {
    recentMenu_->clear();
    const QStringList mru = appSettings().value(u"mru/batch"_s).toStringList();
    if (mru.isEmpty()) {
        recentMenu_->addAction(tr_("(no recent file)"))->setEnabled(false);
        return;
    }
    for (const QString& f : mru) {
        recentMenu_->addAction(native(f), this, [this, f] { openBatchFile(f); });
    }
}
