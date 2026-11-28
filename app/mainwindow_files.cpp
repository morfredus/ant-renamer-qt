#include "mainwindow_internal.h"

#include <QEventLoop>
#include <QRandomGenerator>
#ifdef Q_OS_LINUX
#include <cerrno>
#include <sys/xattr.h>
#endif

// MainWindow: the files list (add, remove, sort, move, drop) and the preview of one file.

namespace {

// Flatpak: the document portal keeps the real (host) path of an exported entry in the xattr user.document-portal.host-path.
// Empty when unknown (not an exported entry, or not Linux).
QString portalHostPath(const QString& docPath) {
#ifdef Q_OS_LINUX
    const QByteArray p = QFile::encodeName(docPath);
    char buf[4096];
    const ssize_t n = getxattr(p.constData(), "user.document-portal.host-path", buf, sizeof buf);
    if (n <= 0) {
        if (qEnvironmentVariableIsSet("AR_DEBUG_DROP")) {
            qWarning() << "host-path xattr not readable for" << docPath << "errno" << errno;
        }
        return {};
    }
    return QString::fromLocal8Bit(buf, int(n));
#else
    Q_UNUSED(docPath);
    return {};
#endif
}

// Used only as the starting folder of the FileChooser portal (it may ignore it): empty when unknown, so nothing changes.
// For a file the start is its folder, for a folder the folder itself.
QString portalStartDir(const QString& docPath) {
    const QString host = portalHostPath(docPath);
    if (host.isEmpty()) {
        return {};
    }
    const QString start = QFileInfo(docPath).isDir() ? host : QFileInfo(host).absolutePath();
    if (qEnvironmentVariableIsSet("AR_DEBUG_DROP")) {
        qWarning() << "picker start folder for" << docPath << "=" << start;
    }
    return start;
}

// Flatpak: the folders already given to the program with the FileChooser portal are kept by the system between sessions, and the
// sandbox sees them as $XDG_RUNTIME_DIR/doc/<id>/<folder> (read and write; a folder given by a drop is listed there too, but
// read only, so it is skipped: it could not be renamed in). Where hostPath (a real path: a dropped file or folder)
// lies inside one of them, this is its path in the sandbox, so nothing has to be asked again. The deepest folder wins.
// Empty when no granted folder holds it (nothing changes then).
QString grantedPortalPath(const QString& hostPath) {
#ifdef Q_OS_LINUX
    const QString runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtime.isEmpty() || hostPath.isEmpty()) {
        return {};
    }
    QString best;
    qsizetype bestLen = -1;
    const QDir docs(runtime + u"/doc"_s);
    for (const QString& id : docs.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QDir entryDir(docs.filePath(id));
        for (const QString& name : entryDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
            const QString doc = entryDir.filePath(name);
            if (!QFileInfo(doc).isWritable()) {
                continue;  // given by a drop: read only (dr-xr-xr-x), and gone with the session. Only the picker's grants write.
            }
            QString base = portalHostPath(doc);
            if (base.isEmpty()) {
                continue;
            }
            while (base.endsWith(u'/')) {
                base.chop(1);  // "/" granted: empty base, every path lies inside
            }
            if ((hostPath != base && !hostPath.startsWith(base + u'/')) || base.size() <= bestLen) {
                continue;
            }
            best = doc + hostPath.mid(base.size());
            bestLen = base.size();
        }
    }
    if (qEnvironmentVariableIsSet("AR_DEBUG_DROP")) {
        qWarning() << "granted folder for" << hostPath << "=" << best;
    }
    return best;
#else
    Q_UNUSED(hostPath);
    return {};
#endif
}

}  // namespace

void MainWindow::portalFolderResponse(uint code, const QVariantMap& results) {
    portalUris_ = code == 0 ? results.value(u"uris"_s).toStringList() : QStringList();
    portalDone_ = true;
    if (portalLoop_) {
        portalLoop_->quit();
    }
}

QString MainWindow::pickFolder(const QString& startHost) {
#ifdef AR_HAVE_DBUS
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!startHost.isEmpty() && bus.isConnected() && !portalLoop_) {
        const QString token = u"antrenamer%1"_s.arg(QRandomGenerator::global()->generate());
        QString sender = bus.baseService().mid(1);
        sender.replace(u'.', u'_');
        const QString request = u"/org/freedesktop/portal/desktop/request/%1/%2"_s.arg(sender, token);
        const QString service = u"org.freedesktop.portal.Desktop"_s, iface = u"org.freedesktop.portal.Request"_s;
        portalDone_ = false;
        portalUris_.clear();
        if (bus.connect(service, request, iface, u"Response"_s, this, SLOT(portalFolderResponse(uint,QVariantMap)))) {
            QVariantMap opts;
            opts.insert(u"handle_token"_s, token);
            opts.insert(u"directory"_s, true);
            opts.insert(u"modal"_s, true);
            opts.insert(u"current_folder"_s, QFile::encodeName(startHost) + '\0');  // bytestring, NUL ended
            QDBusMessage call = QDBusMessage::createMethodCall(service, u"/org/freedesktop/portal/desktop"_s,
                                                               u"org.freedesktop.portal.FileChooser"_s, u"OpenFile"_s);
            call << QString() << tr_("Select folder") << opts;
            const QDBusMessage reply = bus.call(call, QDBus::Block, 5000);
            if (reply.type() == QDBusMessage::ReplyMessage) {
                QEventLoop loop;
                portalLoop_ = &loop;
                if (!portalDone_) {
                    loop.exec(QEventLoop::ExcludeUserInputEvents);
                }
                portalLoop_ = nullptr;
                bus.disconnect(service, request, iface, u"Response"_s, this, SLOT(portalFolderResponse(uint,QVariantMap)));
                return portalUris_.isEmpty() ? QString() : QUrl(portalUris_.first()).toLocalFile();  // empty: cancelled
            }
            bus.disconnect(service, request, iface, u"Response"_s, this, SLOT(portalFolderResponse(uint,QVariantMap)));
        }
    }
#endif
    return QFileDialog::getExistingDirectory(this, tr_("Select folder"), startHost, QFileDialog::ShowDirsOnly);
}

// --- files ---
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
    // refused by the preview (Info) or by the last run (a reason in the description), no new name to show
    markRefusedRow(item, f->preview.isEmpty() && (f->status() == State::Info || !f->description().isEmpty()), palette());
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

// The row of the files list that shows `f`.
QTreeWidgetItem* MainWindow::addFileRow(const File& f) {
    auto* item = new FileItem;
    item->setData(0, IdRole, f.id());
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
    fileView_->addTopLevelItem(item);
    rows_.insert(f.id(), item);
    return item;
}

void MainWindow::addFile(const QString& path) {
    File& f = files_.add(path);
    f.hidden = isHiddenOrSystem(QFileInfo(path));
    updateItem(addFileRow(f));
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

// With no choice every path is added as is; otherwise folders are expanded.
void MainWindow::addFiles(const QStringList& paths, const AddChoice* choice, bool popup) {
    if (worker_) {
        return;  // the list belongs to the worker thread during a run
    }
    setStatus(0, tr_("Adding files"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    fileView_->setUpdatesEnabled(false);
    QStringList portalFiles;  // Flatpak: files given alone, they cannot be renamed until their folder is granted
    QString emptyDir;  // first folder that brought nothing: without a word, an empty folder looks like a failure
    for (const QString& p : paths) {
        const QFileInfo fi(p);
        if (choice && fi.isDir()) {
            const size_t before = files_.size();
            if (choice->folders) {
                addFile(native(p));
            }
            addFolderContents(p, *choice);
            if (files_.size() == before && emptyDir.isEmpty()) {
                emptyDir = fi.fileName().isEmpty() ? native(p) : fi.fileName();
            }
        } else if (!choice || (choice->files && fi.isFile())) {
            if (ren::isPortalSingleFile(QDir::fromNativeSeparators(p))) {
                // Flatpak: its folder may already have been given to the program (kept by the system): then nothing is asked.
                const QString host = portalHostPath(p);
                const QString granted = host.isEmpty() ? QString() : grantedPortalPath(host);
                if (!granted.isEmpty() && QFileInfo(granted).isFile()) {
                    addFile(native(granted));
                } else {
                    portalFiles << p;
                }
            } else {
                addFile(native(p));
            }
        }
    }
    bool portalFile = false;
    if (!portalFiles.isEmpty()) {
        // Flatpak only (isPortalSingleFile is false elsewhere). The sandbox has no access to the home folder: the folder is asked
        // through the FileChooser portal, which keeps the permission. Its files are then reached by their real folder, not
        // through the private folder of the document portal, and can be renamed.
        fileView_->setUpdatesEnabled(true);
        QApplication::restoreOverrideCursor();
        QString dir;
        if (QMessageBox::question(this, QStringLiteral("Ant Renamer Qt"),
                                  tr_("Files given one by one to the Flatpak sandbox cannot be renamed. Give the program access to the folder that contains them (the system keeps this permission)?")) ==
            QMessageBox::Yes) {
            dir = pickFolder(portalStartDir(portalFiles.first()));
        }
        QApplication::setOverrideCursor(Qt::WaitCursor);
        fileView_->setUpdatesEnabled(false);
        for (const QString& p : std::as_const(portalFiles)) {
            const QString inDir = dir.isEmpty() ? QString() : QDir(dir).filePath(QFileInfo(p).fileName());
            if (!inDir.isEmpty() && QFileInfo(inDir).isFile()) {
                addFile(native(inDir));
            } else {
                addFile(native(p));
                portalFile = true;
            }
        }
    }
    fileView_->setUpdatesEnabled(true);
    QApplication::restoreOverrideCursor();
    setStatus();
    if (!emptyDir.isEmpty()) {
        const QString msg = tr_("No file found in %1").arg(emptyDir);
        setStatus(-1, msg);
        addLog(msg, State::Info);  // the status text goes away at the next action; the log keeps it
        if (popup) {
            QMessageBox::information(this, QStringLiteral("Ant Renamer Qt"), msg);
        }
    }
    updateSamplePreview();
    if (portalFile) {
        QMessageBox::warning(this, QStringLiteral("Ant Renamer Qt"),
                             tr_("Files given one by one to the Flatpak sandbox (drag and drop, file picker) cannot be renamed: the program will refuse, nothing will be changed. Add the folder that contains them instead (Add folders, or drag the folder)."));
    }
}

// Paths sent by the Explorer context menu (or "Open with"): like a drop, with the same window. Explorer starts one process
// per selected item, so the paths arrive one by one: the question is asked once, when no more comes for a moment.
void MainWindow::addShellPaths(const QStringList& paths) {
    if (paths.isEmpty()) {
        return;
    }
    shellPaths_ += paths;
    if (!shellTimer_) {
        shellTimer_ = new QTimer(this);
        shellTimer_->setSingleShot(true);
        connect(shellTimer_, &QTimer::timeout, this, &MainWindow::importShellPaths);
    }
    shellTimer_->start(400);
}

void MainWindow::importShellPaths() {
    if (worker_) {
        shellTimer_->start(400);  // a run is going on: try again afterwards, the files must not be lost
        return;
    }
    QStringList paths;
    paths.swap(shellPaths_);
    paths.removeDuplicates();
    showTab(0);
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
    importDropped(paths, true);
}

// The question is asked once the drop is over: a modal window opened inside the drop handler stays behind the file
// manager, which is still waiting for the end of its drag.
void MainWindow::filesDropped(const QStringList& paths) {
    if (worker_ || paths.isEmpty()) {
        return;
    }
    QTimer::singleShot(0, this, [this, paths] { importDropped(paths); });
}

void MainWindow::importDropped(const QStringList& paths, bool fromShell) {
    if (worker_) {
        return;
    }
    // without the question, selected files of the Explorer menu are always added
    AddChoice c{fromShell || appOptions().addFiles, appOptions().addFolders, appOptions().recursive, appOptions().addHidden};
    if (!appOptions().dragNoAsk) {
        bringToFront(this);
        DragDropDialog dlg(this);
        dlg.show();
        bringToFront(&dlg);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }
        c = dlg.choice();
        rememberAddChoice(c);
        appOptions().dragNoAsk = dlg.dontAskAgain();
        appOptions().save();
    }
    // Flatpak only: a dropped folder is /run/user/<uid>/doc/<id>/<folder>, given by the file manager, often read-only and gone
    // at the next session. The same folder chosen with the FileChooser portal is writable and kept, so it is asked once and
    // replaces the dropped one (the names must match, else the dropped one stays).
    QStringList list = paths;
    for (QString& p : list) {
        const QFileInfo fi(p);
        if (!fi.isDir() || !ren::isPortalSingleFile(QDir::fromNativeSeparators(p))) {
            continue;
        }
        // Already given to the program (the folder itself, or a folder that holds it): the writable path is used, no question.
        const QString host = portalHostPath(p);
        const QString granted = host.isEmpty() ? QString() : grantedPortalPath(host);
        if (!granted.isEmpty() && QFileInfo(granted).isDir()) {
            p = granted;
            continue;
        }
        if (QMessageBox::question(this, QStringLiteral("Ant Renamer Qt"),
                                  tr_("The folder \"%1\" was given to the Flatpak sandbox by drag and drop: it can be read, but renaming may fail. Select it again to give the program lasting access (the system keeps this permission)?").arg(fi.fileName())) !=
            QMessageBox::Yes) {
            continue;
        }
        const QString dir = pickFolder(portalStartDir(p));
        if (!dir.isEmpty() && QFileInfo(dir).fileName() == fi.fileName()) {
            p = dir;
        } else if (!dir.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Ant Renamer Qt"),
                                 tr_("This is not the same folder: the dropped folder is kept."));
        }
    }
    addFiles(list, &c, true);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (hasDroppablePaths(e->mimeData())) {
        e->acceptProposedAction();
    }
}
void MainWindow::dropEvent(QDropEvent* e) {
    const QStringList paths = droppedPaths(e->mimeData());
    if (qEnvironmentVariableIsSet("AR_DEBUG_DROP")) {  // to see what a file manager really sends
        qWarning() << "drop formats:" << e->mimeData()->formats() << "paths:" << paths;
    }
    if (runsInFlatpak() && !paths.isEmpty()) {
        // The sandbox only sees what the portal gave: a file manager that sends host paths (no portal key) gives paths that
        // do not exist here, and nothing could be added without a word.
        const bool anyVisible = std::any_of(paths.cbegin(), paths.cend(), [](const QString& p) { return QFileInfo::exists(p); });
        if (!anyVisible) {
            QMessageBox::warning(this, QStringLiteral("Ant Renamer Qt"),
                                 tr_("The Flatpak sandbox cannot see what this file manager dropped. Use Add folders or Add files instead."));
            return;
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
    filterRemoved_.clear();  // "Remove all" empties the list for good
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

// A file added at a given row of the list (past the end: at the end).
void MainWindow::insertFileAt(const QString& path, int row) {
    File& f = files_.add(path);
    f.hidden = isHiddenOrSystem(QFileInfo(path));
    QTreeWidgetItem* item = addFileRow(f);
    updateItem(item);
    const int last = fileView_->topLevelItemCount() - 1;
    if (row >= 0 && row < last) {
        fileView_->insertTopLevelItem(row, fileView_->takeTopLevelItem(last));
    }
}

// "Set dates from EXIF": the modification date of each file becomes its EXIF shooting date. Never automatic: a table shows the
// old and the new date of every file, with the safeguards written out, and nothing happens before the button is pressed.
// The run goes through the worker (log, history, undo like a renaming).
void MainWindow::setDatesFromExif() {
    if (worker_) {
        return;
    }
    if (files_.size() == 0) {
        QMessageBox::information(this, tr_("Set the dates from EXIF"), tr_("No file: add files first."));
        return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(tr_("Set the dates from EXIF"));
    auto* lay = new QVBoxLayout(&dlg);
    auto* intro = new QLabel(
        tr_("The modification date of each file below will become the shooting date written in its EXIF data. Only the date "
            "kept by the file system changes: the content of the file, its EXIF data and its tags are not touched. A file with "
            "no EXIF shooting date is left alone.\n\nThe modification date is not a proof of authenticity. For originals that "
            "must stay evidence, work on copies (option \"Copy instead of rename\"). This can be undone from the history."));
    intro->setWordWrap(true);
    lay->addWidget(intro);
    auto* tree = new QTreeWidget;
    tree->setRootIsDecorated(false);
    tree->setHeaderLabels({tr_("File"), tr_("Modification date"), tr_("EXIF shooting date"), tr_("Result")});
    lay->addWidget(tree, 1);
    QApplication::setOverrideCursor(Qt::WaitCursor);  // the EXIF of every picture is read
    int changes = 0;
    const QString fmt = u"yyyy-MM-dd HH:mm:ss"_s;
    const std::vector<ren::DatePlan> plans = ren::planDates(files_);
    for (size_t i = 0; i < files_.size(); ++i) {
        const File& f = files_.at(i);
        const ren::DatePlan& p = plans[i];
        QString result;
        switch (p.kind) {
        case ren::DatePlan::Kind::Change:
            result = p.sidecars.isEmpty() ? tr_("will be changed") : tr_("will be changed, with %1 sidecar file(s)").arg(p.sidecars.size());
            ++changes;
            break;
        case ren::DatePlan::Kind::Sidecar:
            result = tr_("follows its picture");
            break;
        case ren::DatePlan::Kind::Same:
            result = tr_("already right");
            break;
        case ren::DatePlan::Kind::NoDate:
            result = tr_("no EXIF shooting date");
            break;
        case ren::DatePlan::Kind::Skipped:
            result = f.isFolder() ? tr_("a folder has no EXIF date") : tr_("hidden file");
            break;
        }
        auto* it = new QTreeWidgetItem(tree, {native(f.filePath()), p.current.isValid() ? p.current.toString(fmt) : QString(),
                                              p.shot.isValid() ? p.shot.toString(fmt) : QString(), result});
        if (p.kind != ren::DatePlan::Kind::Change) {
            for (int c = 0; c < 4; ++c) {
                it->setForeground(c, palette().color(QPalette::Disabled, QPalette::Text));
            }
        }
    }
    QApplication::restoreOverrideCursor();
    for (int c = 1; c < 4; ++c) {
        tree->resizeColumnToContents(c);
    }
    tree->setColumnWidth(0, 330);
    lay->addWidget(new QLabel(tr_("%1 file(s) will get a new modification date.").arg(changes)));
    auto* box = new QDialogButtonBox(QDialogButtonBox::Cancel);
    QPushButton* go = box->addButton(tr_("Set the dates"), QDialogButtonBox::AcceptRole);
    go->setEnabled(changes > 0);
    box->button(QDialogButtonBox::Cancel)->setDefault(true);  // Enter does not change dates by reflex
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(box);
    dlg.resize(1000, 480);  // wide enough for the longest result ("will be changed, with 2 sidecar file(s)") in French too
    if (dlg.exec() == QDialog::Accepted) {
        run(Op::SetDates);
    }
}

// Keeps (or removes) the files that match a filter on a date, a time of the day or the size. The files are only taken out of
// the list: nothing is touched on the disk. The dialog shows the last criteria again and, when the last filter took files
// out, offers to put them back first, so that a filter that left nothing can be corrected. "Clear the filter" (or no
// criterion left ticked while the files are put back) gives every file back and filters nothing. A file comes back right
// after the file that was before it when it was taken out (at the end if that one is gone).
void MainWindow::filterFiles() {
    if (worker_) {
        return;
    }
    if (files_.size() == 0 && filterRemoved_.isEmpty()) {
        QMessageBox::information(this, tr_("Filter files"), tr_("No file to filter: add files first."));
        return;
    }
    FilterDialog dlg(this, lastFilter_, lastKeep_, int(filterRemoved_.size()));
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const ren::FileFilter flt = dlg.filter();
    const bool clear = dlg.clearRequested() || (flt.isEmpty() && dlg.putBack());
    if (flt.isEmpty() && !clear) {
        QMessageBox::information(this, tr_("Filter files"), tr_("Nothing to filter: tick at least one criterion."));
        return;
    }
    const bool keep = dlg.keepMatching();
    QApplication::setOverrideCursor(Qt::WaitCursor);  // the EXIF date needs a read of every picture
    if (dlg.putBack() || clear) {
        QString lastAnchor;
        int nextRow = -1;
        for (const FilterRemoved& r : std::as_const(filterRemoved_)) {
            if (!QFileInfo::exists(r.path)) {
                continue;
            }
            int row = fileView_->topLevelItemCount();  // the anchor is gone: at the end
            if (nextRow >= 0 && r.anchor == lastAnchor) {
                row = nextRow;  // several files in a row come back in their order
            } else if (r.anchor.isEmpty()) {
                row = 0;  // it was at the top of the list
            } else {
                for (int k = 0; k < fileView_->topLevelItemCount(); ++k) {
                    const File* f = fileOf(fileView_->topLevelItem(k));
                    if (f && f->filePath() == r.anchor) {
                        row = k + 1;
                        break;
                    }
                }
            }
            insertFileAt(r.path, row);
            lastAnchor = r.anchor;
            nextRow = row + 1;
        }
        filterRemoved_.clear();
        renumberFiles();
    }
    if (clear) {
        QApplication::restoreOverrideCursor();
        lastFilter_ = {};
        lastKeep_ = true;
        setStatus();
        updateSamplePreview();
        return;
    }
    // forward pass: each file that goes remembers the kept file before it
    QList<FilterRemoved> removed;
    QList<QTreeWidgetItem*> gone;
    QString lastKept;
    for (int i = 0; i < fileView_->topLevelItemCount(); ++i) {
        QTreeWidgetItem* it = fileView_->topLevelItem(i);
        File* f = fileOf(it);
        if (!f) {
            continue;
        }
        if (flt.matches(*f) != keep) {
            removed.append({f->filePath(), lastKept});
            gone.append(it);
        } else {
            lastKept = f->filePath();
        }
    }
    for (QTreeWidgetItem* it : std::as_const(gone)) {
        if (File* f = fileOf(it)) {
            rows_.remove(f->id());
            files_.remove(f);
        }
        delete it;
    }
    QApplication::restoreOverrideCursor();
    filterRemoved_ += removed;
    lastFilter_ = flt;
    lastKeep_ = keep;
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

// --- list helpers ---
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
        default:
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

// --- sample preview (one file, current action) ---
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
