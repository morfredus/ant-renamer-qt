#include "dialogs.h"

#include "appsettings.h"
#include "icons.h"
#include "lngtranslator.h"
#include "shellmenu.h"
#include "updatecheck.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QDir>
#include <QFileDialog>
#include <QListWidget>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QTreeView>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QTimeEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

QString displayVersion() {
    QString v = QApplication::applicationVersion();
#ifdef AR_BUILD_ID
    if (v.contains(u'-')) {
        return v + u" ("_s + QStringLiteral(AR_BUILD_ID) + u')';
    }
#endif
    return v;
}

namespace {

QDialogButtonBox* standardButtons(QDialog* d) {
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(bb, &QDialogButtonBox::accepted, d, &QDialog::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, d, &QDialog::reject);
    return bb;
}

}  // namespace


// --- Add folders ---
AddFoldersDialog::AddFoldersDialog(const QString& startFolder, QWidget* parent)
    : QDialog(parent), portal_(QFile::exists(u"/.flatpak-info"_s)) {
    setWindowTitle(tr("Select folders to add"));
    setWindowIcon(tbIcon(TbAddFolder));
    resize(450, 520);

    auto* v = new QVBoxLayout(this);

    if (!portal_) {
        model_ = new QFileSystemModel(this);
        model_->setFilter(QDir::Dirs | QDir::Drives | QDir::NoDotAndDotDot);
        model_->setRootPath(QString());
        tree_ = new QTreeView;
        tree_->setModel(model_);
        tree_->setRootIndex(model_->index(QString()));
        for (int c = 1; c < model_->columnCount(); ++c) {
            tree_->hideColumn(c);
        }
        tree_->setHeaderHidden(true);
        tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        v->addWidget(tree_, 1);

        path_ = new QLineEdit;
        path_->setPlaceholderText(tr("Or type the path of a folder"));
        auto* pathRow = new QHBoxLayout;
        pathRow->addWidget(path_, 1);
        auto* paste = new QPushButton(tr("Paste from clipboard"));
        connect(paste, &QPushButton::clicked, this, [this] {  // Explorer's "Copy as path" puts the path between quotes
            QString t = QApplication::clipboard()->text().trimmed();
            if (t.size() > 1 && t.startsWith(u'"') && t.endsWith(u'"')) {
                t = t.mid(1, t.size() - 2);
            }
            path_->setText(t);
        });
        pathRow->addWidget(paste);
        v->addLayout(pathRow);

        for (auto* c : {boxes_.files, boxes_.folders, boxes_.recursive, boxes_.hidden}) {
            v->addWidget(c);
        }
        v->addWidget(standardButtons(this));

        const QModelIndex start = model_->index(startFolder);
        if (start.isValid()) {
            tree_->setCurrentIndex(start);
            tree_->scrollTo(start);
        }
        return;
    }

    view_ = new QListWidget;
    view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    v->addWidget(view_, 1);

    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton(tr("Add folder..."));
    auto* remove = new QPushButton(tr("Remove selected"));

    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    v->addLayout(buttons);

    connect(add, &QPushButton::clicked, this, [this, startFolder] {
        QString start = folders_.isEmpty() ? startFolder : folders_.last();

        if (!QFileInfo(start).isDir()) {
            start = QDir::homePath();
        }

        const QString folder = QFileDialog::getExistingDirectory(
            this,
            tr("Select folder"),
            start,
            QFileDialog::ShowDirsOnly
        );

        if (folder.isEmpty()) {
            if (folders_.isEmpty()) {
                reject();
            }
            return;
        }

        const QString path = QDir::cleanPath(folder);

        if (folders_.contains(path)) {
            return;
        }

        folders_ << path;

        auto* item = new QListWidgetItem(QDir::toNativeSeparators(path));
        item->setData(Qt::UserRole, path);
        view_->addItem(item);
    });

    connect(remove, &QPushButton::clicked, this, [this] {
        const auto selected = view_->selectedItems();

        for (QListWidgetItem* item : selected) {
            folders_.removeOne(item->data(Qt::UserRole).toString());
            delete view_->takeItem(view_->row(item));
        }
    });

    for (auto* c : {boxes_.files, boxes_.folders, boxes_.recursive, boxes_.hidden}) {
        v->addWidget(c);
    }

    v->addWidget(standardButtons(this));

    // Flatpak: no tree, the list starts empty, so open the portal picker at once.
    QTimer::singleShot(0, add, &QPushButton::click);
}

QStringList AddFoldersDialog::folders() const {
    if (portal_) {
        return folders_;
    }
    QStringList l;
    for (const QModelIndex& i : tree_->selectionModel()->selectedRows(0)) {
        l << model_->filePath(i);
    }
    const QString typed = QDir::cleanPath(path_->text().trimmed());
    if (!path_->text().trimmed().isEmpty() && QFileInfo(typed).isDir() && !l.contains(typed)) {
        l << typed;
    }
    return l;
}

AddChoice AddFoldersDialog::choice() const {
    return boxes_.choice();
}

AddChoiceBoxes::AddChoiceBoxes()
    : files(new QCheckBox(QDialog::tr("Add files of selected folders"))),
      folders(new QCheckBox(QDialog::tr("Add selected folders"))),
      recursive(new QCheckBox(QDialog::tr("Apply these two options also to subfolders"))),
      hidden(new QCheckBox(QDialog::tr("Include hidden and system files when adding a folder"))) {
    const AppOptions& o = appOptions();
    files->setChecked(o.addFiles);
    folders->setChecked(o.addFolders);
    recursive->setChecked(o.recursive);
    hidden->setChecked(o.addHidden);
}

void rememberAddChoice(const AddChoice& c) {
    AppOptions& o = appOptions();
    o.addFiles = c.files;
    o.addFolders = c.folders;
    o.recursive = c.recursive;
    o.addHidden = c.hidden;
}

AddChoice AddChoiceBoxes::choice() const {
    return {files->isChecked(), folders->isChecked(), recursive->isChecked(), hidden->isChecked()};
}

// --- Drag & drop ---
DragDropDialog::DragDropDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Drag & Drop Options"));
    setWindowIcon(tbIcon(TbAddFiles));
    auto* v = new QVBoxLayout(this);
    v->addWidget(new QLabel(tr("Options to use to add files that have been dropped:")));
    notAgain_ = new QCheckBox(tr("Do not show this window again (keep selected options for next times)"));
    for (auto* c : {boxes_.files, boxes_.folders, boxes_.recursive, boxes_.hidden}) {
        v->addWidget(c);
    }
    v->addSpacing(8);
    v->addWidget(notAgain_);
    v->addWidget(standardButtons(this));
}

AddChoice DragDropDialog::choice() const {
    return boxes_.choice();
}
bool DragDropDialog::dontAskAgain() const {
    return notAgain_->isChecked();
}

// --- Filter files ---
struct FilterDialog::Ui {
    QComboBox* kind;
    QCheckBox *useFrom, *useTo, *useTime, *useMin, *useMax;
    QDateTimeEdit *from, *to;
    QTimeEdit *timeFrom, *timeTo;
    QSpinBox *minSize, *maxSize;
    QComboBox *minUnit, *maxUnit;
    QRadioButton *keep, *remove;
    QCheckBox* putBack = nullptr;
    bool clear = false;
};

FilterDialog::FilterDialog(QWidget* parent, const ren::FileFilter& previous, bool keep, int removed)
    : QDialog(parent), ui_(std::make_unique<Ui>()) {
    setWindowTitle(tr("Filter files"));
    auto* v = new QVBoxLayout(this);
    if (removed > 0) {
        ui_->putBack = new QCheckBox(tr("Put back the %1 file(s) removed by the previous filter first").arg(removed));
        ui_->putBack->setChecked(true);
        v->addWidget(ui_->putBack);
    }
    auto* g = new QGridLayout;
    v->addLayout(g);
    int row = 0;
    ui_->kind = new QComboBox;
    ui_->kind->addItems({tr("Modification date"), tr("Creation date"), tr("Shooting date (EXIF; else modification date)")});
    g->addWidget(new QLabel(tr("Date to look at:")), row, 0);
    g->addWidget(ui_->kind, row++, 1, 1, 2);

    const QDateTime now = QDateTime::currentDateTime();
    ui_->useFrom = new QCheckBox(tr("From"));
    ui_->from = new QDateTimeEdit(QDateTime(now.date().addDays(-7), QTime(0, 0)));
    ui_->useTo = new QCheckBox(tr("To"));
    ui_->to = new QDateTimeEdit(QDateTime(now.date(), QTime(23, 59, 59)));
    for (auto* e : {ui_->from, ui_->to}) {
        e->setDisplayFormat(QLocale().dateTimeFormat(QLocale::ShortFormat));
        e->setCalendarPopup(true);
    }
    g->addWidget(ui_->useFrom, row, 0);
    g->addWidget(ui_->from, row++, 1, 1, 2);
    g->addWidget(ui_->useTo, row, 0);
    g->addWidget(ui_->to, row++, 1, 1, 2);

    ui_->useTime = new QCheckBox(tr("Time of the day, from"));
    ui_->timeFrom = new QTimeEdit(QTime(14, 0));
    ui_->timeTo = new QTimeEdit(QTime(15, 0));
    for (auto* e : {ui_->timeFrom, ui_->timeTo}) {
        e->setDisplayFormat(u"HH:mm:ss"_s);
    }
    g->addWidget(ui_->useTime, row, 0);
    g->addWidget(ui_->timeFrom, row, 1);
    auto* toLabel = new QLabel(tr("to"));
    auto* th = new QHBoxLayout;
    th->addWidget(toLabel);
    th->addWidget(ui_->timeTo);
    g->addLayout(th, row++, 2);

    auto sizeRow = [&](QCheckBox*& use, QSpinBox*& spin, QComboBox*& unit, const QString& label) {
        use = new QCheckBox(label);
        spin = new QSpinBox;
        spin->setRange(0, 1000000);
        unit = new QComboBox;
        unit->addItems({u"KB"_s, u"MB"_s, u"GB"_s});
        unit->setCurrentIndex(1);
        spin->setValue(1);
        g->addWidget(use, row, 0);
        g->addWidget(spin, row, 1);
        g->addWidget(unit, row++, 2);
    };
    sizeRow(ui_->useMin, ui_->minSize, ui_->minUnit, tr("Size at least"));
    sizeRow(ui_->useMax, ui_->maxSize, ui_->maxUnit, tr("Size at most"));

    // a criterion is used only when its box is ticked
    // (a QList, not an initializer_list: the lambda outlives the call and must own the pointers)
    auto link = [](QCheckBox* box, const QList<QWidget*>& ws) {
        for (QWidget* w : ws) {
            w->setEnabled(box->isChecked());
        }
        QObject::connect(box, &QCheckBox::toggled, box, [ws](bool on) {
            for (QWidget* w : ws) {
                w->setEnabled(on);
            }
        });
    };
    link(ui_->useFrom, {ui_->from});
    link(ui_->useTo, {ui_->to});
    link(ui_->useTime, {ui_->timeFrom, ui_->timeTo});
    link(ui_->useMin, {ui_->minSize, ui_->minUnit});
    link(ui_->useMax, {ui_->maxSize, ui_->maxUnit});

    v->addSpacing(6);
    ui_->keep = new QRadioButton(tr("Keep the files that match"));
    ui_->remove = new QRadioButton(tr("Remove the files that match"));
    (keep ? ui_->keep : ui_->remove)->setChecked(true);
    v->addWidget(ui_->keep);
    v->addWidget(ui_->remove);
    QDialogButtonBox* buttons = standardButtons(this);
    if (removed > 0) {  // a way to get every file of the list back without choosing a criterion
        auto* clearBtn = buttons->addButton(tr("Clear the filter"), QDialogButtonBox::ActionRole);
        connect(clearBtn, &QPushButton::clicked, this, [this] {
            ui_->clear = true;
            accept();
        });
    }
    v->addWidget(buttons);

    // the last criteria again, to correct them (set after link(): ticking a box enables its fields)
    ui_->kind->setCurrentIndex(int(previous.kind));
    if (previous.from) {
        ui_->useFrom->setChecked(true);
        ui_->from->setDateTime(*previous.from);
    }
    if (previous.to) {
        ui_->useTo->setChecked(true);
        ui_->to->setDateTime(*previous.to);
    }
    if (previous.timeFrom || previous.timeTo) {
        ui_->useTime->setChecked(true);
        ui_->timeFrom->setTime(previous.timeFrom.value_or(QTime(0, 0)));
        ui_->timeTo->setTime(previous.timeTo ? previous.timeTo->addMSecs(-999) : QTime(23, 59, 59));
    }
    auto showSize = [](const std::optional<qint64>& size, QCheckBox* use, QSpinBox* spin, QComboBox* unit) {
        if (!size) {
            return;
        }
        use->setChecked(true);
        const int u = (*size % (1ll << 30) == 0) ? 2 : (*size % (1ll << 20) == 0) ? 1 : 0;  // the biggest unit that is exact
        unit->setCurrentIndex(u);
        spin->setValue(int(qMax<qint64>(*size >> (10 * (u + 1)), 0)));
    };
    showSize(previous.minSize, ui_->useMin, ui_->minSize, ui_->minUnit);
    showSize(previous.maxSize, ui_->useMax, ui_->maxSize, ui_->maxUnit);
}

FilterDialog::~FilterDialog() = default;

ren::FileFilter FilterDialog::filter() const {
    ren::FileFilter f;
    f.kind = static_cast<ren::FileFilter::DateKind>(ui_->kind->currentIndex());
    if (ui_->useFrom->isChecked()) {
        f.from = ui_->from->dateTime();
    }
    if (ui_->useTo->isChecked()) {
        f.to = ui_->to->dateTime();
    }
    if (ui_->useTime->isChecked()) {
        f.timeFrom = ui_->timeFrom->time();
        f.timeTo = ui_->timeTo->time().addMSecs(999);  // up to the end of the last second
    }
    auto bytes = [](const QSpinBox* s, const QComboBox* u) {
        return qint64(s->value()) << (10 * (u->currentIndex() + 1));
    };
    if (ui_->useMin->isChecked()) {
        f.minSize = bytes(ui_->minSize, ui_->minUnit);
    }
    if (ui_->useMax->isChecked()) {
        f.maxSize = bytes(ui_->maxSize, ui_->maxUnit);
    }
    return f;
}

bool FilterDialog::keepMatching() const {
    return ui_->keep->isChecked();
}

bool FilterDialog::clearRequested() const {
    return ui_->clear;
}

bool FilterDialog::putBack() const {
    return ui_->putBack && ui_->putBack->isChecked();
}

// --- Options ---
struct OptionsDialog::Ui {
    QCheckBox *realtime, *resizeCols, *icons, *forceFont, *autoComplete, *dragNoAsk, *launch, *addHidden, *naturalSort, *newUi,
        *forceDir, *detectAbs, *copy, *genLog, *folderExt, *sidecar, *numDup, *dst, *portable, *shellMenu;
    QCheckBox *dropFiles, *dropFolders, *dropRecursive;
    QComboBox* pageLayout = nullptr;
    QComboBox* undoButton = nullptr;
    QSpinBox *historyMax, *fontSize;
    QComboBox* theme;
    QCheckBox *checkUpdates, *checkPre;
    QFontComboBox* font;
    QComboBox* iconSet;
    QCheckBox *startSwitch, *startClearLog, *finishSwitch, *finishClearFiles, *finishClearBatch, *saveLog, *saveLogAppend;
    QComboBox *startTab, *finishTab;
    QLineEdit* logFile;
    QTreeWidget* languages;
    QLabel *langVersion, *langAuthors, *langComments;
};

OptionsDialog::OptionsDialog(QWidget* parent) : QDialog(parent), ui_(std::make_unique<Ui>()) {
    setWindowTitle(tr("Options"));
    setWindowIcon(tbIcon(TbOptions));
    resize(520, 440);
    const AppOptions& o = appOptions();
    auto* v = new QVBoxLayout(this);
    auto* tabs = new QTabWidget;
    v->addWidget(tabs);

    auto page = [&](const QString& title) {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        tabs->addTab(w, title);
        return l;
    };
    auto header = [](QVBoxLayout* l, const QString& t) {
        auto* h = new QLabel(u"<b>"_s + t.toHtmlEscaped() + u"</b>"_s);
        l->addSpacing(6);
        l->addWidget(h);
    };
    auto box = [](QVBoxLayout* l, const QString& t, bool on) {
        auto* c = new QCheckBox(t);
        c->setChecked(on);
        l->addWidget(c);
        return c;
    };

    {  // Display
        auto* l = page(tr("Display"));
        header(l, tr("Lists"));
        ui_->realtime = box(l, tr("Real-time update when renaming files"), o.realtime);
        ui_->resizeCols = box(l, tr("Auto-resize columns of Files list"), o.resizeCols);
        ui_->icons = box(l, tr("Show icons in files list, based on file extension"), o.fileIcons);
        ui_->newUi = box(l, tr("Modern interface layout"), o.newUi);
        auto* pl = new QHBoxLayout;
        pl->setContentsMargins(20, 0, 0, 0);
        pl->addWidget(new QLabel(tr("Pages layout (modern interface):")));
        ui_->pageLayout = new QComboBox;
        ui_->pageLayout->addItems({tr("Tabs"), tr("Single screen, in bands"), tr("Single screen, in columns")});
        ui_->pageLayout->setCurrentIndex(qBound(0, o.pageLayout, 2));
        ui_->pageLayout->setEnabled(o.newUi);
        connect(ui_->newUi, &QCheckBox::toggled, ui_->pageLayout, &QWidget::setEnabled);
        pl->addWidget(ui_->pageLayout);
        pl->addStretch();
        l->addLayout(pl);

        auto* th = new QHBoxLayout;
        th->addWidget(new QLabel(tr("Theme:")));
        ui_->theme = new QComboBox;
        ui_->theme->addItems({tr("Follow the system"), tr("Light"), tr("Dark")});
        ui_->theme->setCurrentIndex(qBound(0, o.theme, 2));
        th->addWidget(ui_->theme);
        th->addStretch();
        l->addLayout(th);

        if (!runsInFlatpak()) {
            header(l, tr("Updates"));
        }
        ui_->checkUpdates =
            box(l, tr("Look for a new version at start (once a day, asks GitHub; nothing else is sent)"), o.checkUpdates);
        ui_->checkPre =
            box(l, tr("Also tell me about beta and release candidates (otherwise final versions only)"), o.checkPrereleases);
        if (runsInFlatpak()) {
            ui_->checkUpdates->hide();
            ui_->checkPre->hide();
        }

        header(l, tr("Fonts"));
        ui_->forceFont =
            box(l, tr("Force the following font for controls that have to display special characters:"), o.forceFont);
        auto* fr = new QHBoxLayout;
        ui_->font = new QFontComboBox;
        ui_->font->setCurrentFont(QFont(o.fontName.isEmpty() ? QApplication::font().family() : o.fontName));
        ui_->fontSize = new QSpinBox;
        ui_->fontSize->setRange(6, 32);
        ui_->fontSize->setValue(o.fontSize);
        fr->addWidget(ui_->font, 1);
        fr->addWidget(ui_->fontSize);
        l->addLayout(fr);
        l->addWidget(new QLabel(tr("Note: Noto Sans, DejaVu Sans or Arial Unicode MS is recommended if available")));

        header(l, tr("Drop-down fields"));
        auto* row = new QHBoxLayout;
        row->addWidget(new QLabel(tr("Number of values to keep in history:")));
        ui_->historyMax = new QSpinBox;
        ui_->historyMax->setRange(1, 200);
        ui_->historyMax->setValue(o.historyMax);
        row->addWidget(ui_->historyMax);
        row->addStretch();
        l->addLayout(row);
        ui_->autoComplete = box(l, tr("Autocomplete with history values while typing"), o.autoComplete);

        header(l, tr("Toolbars"));
        auto* ir = new QHBoxLayout;
        ir->addWidget(new QLabel(tr("Icon set:")));
        ui_->iconSet = new QComboBox;
        ui_->iconSet->addItems(iconSetNames());
        ui_->iconSet->setCurrentIndex(o.iconSet);
        ir->addWidget(ui_->iconSet);
        ir->addStretch();
        l->addLayout(ir);
        header(l, tr("Settings storage"));
        ui_->portable = box(l, tr("Portable mode: keep the settings in the program folder"), isPortable());
        l->addStretch();
    }
    {  // Files & folders
        auto* l = page(tr("Files && Folders"));
        header(l, tr("Drag & Drop"));
        ui_->dragNoAsk = box(l, tr("Do not ask what to do, always use the following options:"), o.dragNoAsk);
        ui_->dropFiles = box(l, tr("Add files of selected folders"), o.addFiles);
        ui_->dropFolders = box(l, tr("Add selected folders"), o.addFolders);
        ui_->dropRecursive = box(l, tr("Apply these two options also to subfolders"), o.recursive);
        for (auto* c : {ui_->dropFiles, ui_->dropFolders, ui_->dropRecursive}) {
            c->setContentsMargins(20, 0, 0, 0);
        }
        header(l, tr("List behaviour"));
        ui_->addHidden = box(l, tr("Include hidden and system files when adding a folder"), o.addHidden);
        ui_->launch = box(l, tr("Launch file on double click"), o.launchFile);
        ui_->naturalSort = box(l, tr("Natural sort in files list (2 before 10) instead of alphabetical"), o.naturalSort);
        header(l, tr("Undo button"));
        auto* ru = new QHBoxLayout;
        ru->addWidget(new QLabel(tr("The Undo button:")));
        ui_->undoButton = new QComboBox;
        ui_->undoButton->addItems({tr("undoes the last pass"), tr("opens the history of the passes")});
        ui_->undoButton->setCurrentIndex(qBound(0, o.undoButton, 1));
        ru->addWidget(ui_->undoButton);
        ru->addStretch();
        l->addLayout(ru);
        header(l, tr("Windows Explorer"));
        ui_->shellMenu =
            box(l, tr("Add \"Add to Ant Renamer Qt\" to the context menu of files and folders"), shellMenuInstalled());
#ifdef Q_OS_WIN
        l->addWidget(new QLabel(tr("On Windows 11 the entry is in \"Show more options\" (or Shift+F10).")));
#else
        ui_->shellMenu->setEnabled(false);
#endif
        l->addStretch();
    }
    {  // Processing
        auto* l = page(tr("Processing"));
        header(l, tr("File renaming"));
        ui_->copy = box(l, tr("Copy instead of Rename"), o.copy);
        ui_->genLog = box(l, tr("Generate log"), o.genLog);
        ui_->dst = box(l, tr("DST offset related to file date"), o.dstRelativeToFile);
        ui_->sidecar = box(l, tr("Also rename the sidecar files (.xmp, .dop) of a renamed picture"), o.renameSidecar);
        ui_->numDup = box(l, tr("Number files given the same name by Date and time or EXIF (_01, _02...)"), o.numberDuplicates);
        header(l, tr("Folder rules"));
        ui_->forceDir = box(l, tr("Force directories (allows to create folders if \"\\\" are found in names)"), o.forceDir);
        ui_->detectAbs =
            box(l, tr("Detect a \":\\\" or \"\\\\\" as an absolute path (allows to move files to anywhere)"), o.detectAbsDir);
        ui_->folderExt =
            box(l, tr("Consider the last \".\" in a folder name as extension delimiter, like for files"), o.folderExt);
        l->addStretch();
    }
    {  // Events
        auto* l = page(tr("Events"));
        const QStringList tabNames = {tr("Files list"), tr("Actions"), tr("Log")};
        header(l, tr("When renaming starts"));
        auto* r1 = new QHBoxLayout;
        ui_->startSwitch = new QCheckBox(tr("Switch to:"));
        ui_->startSwitch->setChecked(o.startSwitch >= 0);
        ui_->startTab = new QComboBox;
        ui_->startTab->addItems(tabNames);
        ui_->startTab->setCurrentIndex(qMax(0, o.startSwitch));
        r1->addWidget(ui_->startSwitch);
        r1->addWidget(ui_->startTab);
        r1->addStretch();
        l->addLayout(r1);
        ui_->startClearLog = box(l, tr("Clear log"), o.startClearLog);
        header(l, tr("When renaming ends"));
        auto* r2 = new QHBoxLayout;
        ui_->finishSwitch = new QCheckBox(tr("Switch to:"));
        ui_->finishSwitch->setChecked(o.finishSwitch >= 0);
        ui_->finishTab = new QComboBox;
        ui_->finishTab->addItems(tabNames);
        ui_->finishTab->setCurrentIndex(qMax(0, o.finishSwitch));
        r2->addWidget(ui_->finishSwitch);
        r2->addWidget(ui_->finishTab);
        r2->addStretch();
        l->addLayout(r2);
        ui_->finishClearFiles = box(l, tr("Clear files list"), o.finishClearFiles);
        ui_->finishClearBatch = box(l, tr("Clear batch list"), o.finishClearBatch);
        auto* r3 = new QHBoxLayout;
        ui_->saveLog = new QCheckBox(tr("Save log to file:"));
        ui_->saveLog->setChecked(o.saveLog);
        ui_->logFile = new QLineEdit(o.saveLogFile);
        auto* browse = new QPushButton(u"..."_s);
        browse->setToolTip(tr("Browse..."));
        browse->setMaximumWidth(30);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString csvFilter = tr("CSV file (*.csv)");
            QString filter;
            const QString f = QFileDialog::getSaveFileName(this, tr("Save log"), ui_->logFile->text(),
                                                           csvFilter + u";;"_s + tr("Text files (*.log *.txt)"), &filter);
            if (!f.isEmpty()) {
                ui_->logFile->setText(QDir::toNativeSeparators(withLogExtension(f, filter == csvFilter)));
            }
        });
        r3->addWidget(ui_->saveLog);
        r3->addWidget(ui_->logFile, 1);
        r3->addWidget(browse);
        l->addLayout(r3);
        ui_->saveLogAppend = box(l, tr("Append to current file"), o.saveLogAppend);
        l->addStretch();
    }
    {  // Language: the .lng files of the original program
        auto* l = page(tr("Language"));
        header(l, tr("Language"));
        ui_->languages = new QTreeWidget;
        ui_->languages->setHeaderLabels({tr("Name"), tr("Local name")});
        ui_->languages->setRootIsDecorated(false);
        const QString cur = defaultLanguage();
        const auto langs = availableLanguages(languageDir());
        for (const LangInfo& li : langs) {
            auto* item = new QTreeWidgetItem({li.file, li.localName});
            item->setData(0, Qt::UserRole, li.file);
            item->setData(1, Qt::UserRole, QStringList{li.version, li.authors, li.comments});
            ui_->languages->addTopLevelItem(item);
            if (li.file == cur) {
                ui_->languages->setCurrentItem(item);
            }
        }
        l->addWidget(ui_->languages, 1);
        auto* form = new QFormLayout;
        ui_->langVersion = new QLabel;
        ui_->langAuthors = new QLabel;
        ui_->langComments = new QLabel;
        ui_->langAuthors->setWordWrap(true);
        ui_->langComments->setWordWrap(true);
        form->addRow(tr("Version:"), ui_->langVersion);
        form->addRow(tr("Made by:"), ui_->langAuthors);
        form->addRow(tr("Comments:"), ui_->langComments);
        l->addLayout(form);
        auto showInfo = [this](QTreeWidgetItem* it) {
            const QStringList d = it ? it->data(1, Qt::UserRole).toStringList() : QStringList{{}, {}, {}};
            ui_->langVersion->setText(d.value(0));
            ui_->langAuthors->setText(d.value(1));
            ui_->langComments->setText(d.value(2));
        };
        connect(ui_->languages, &QTreeWidget::currentItemChanged, this, [showInfo](QTreeWidgetItem* it) { showInfo(it); });
        showInfo(ui_->languages->currentItem());
    }
    v->addWidget(standardButtons(this));
}

OptionsDialog::~OptionsDialog() = default;

void OptionsDialog::accept() {
    // storage first: the rest is written where it will be read from next time
    if (ui_->portable->isChecked() != isPortable() && !setPortable(ui_->portable->isChecked())) {
        QMessageBox::warning(this, windowTitle(), tr("Could not write the settings in the program folder."));
    }

    if (ui_->shellMenu->isChecked() != shellMenuInstalled() &&
        !setShellMenu(ui_->shellMenu->isChecked(), tr("Add to Ant Renamer Qt"))) {
        QMessageBox::warning(this, windowTitle(), tr("Could not change the Explorer context menu."));
    }

    AppOptions& o = appOptions();
    o.newUi = ui_->newUi->isChecked();
    o.pageLayout = ui_->pageLayout->currentIndex();
    o.theme = ui_->theme->currentIndex();
    o.checkUpdates = ui_->checkUpdates->isChecked();
    o.checkPrereleases = ui_->checkPre->isChecked();
    o.realtime = ui_->realtime->isChecked();
    o.resizeCols = ui_->resizeCols->isChecked();
    o.fileIcons = ui_->icons->isChecked();
    o.forceFont = ui_->forceFont->isChecked();
    o.fontName = ui_->font->currentFont().family();
    o.fontSize = ui_->fontSize->value();
    o.iconSet = ui_->iconSet->currentIndex();
    o.historyMax = ui_->historyMax->value();
    o.autoComplete = ui_->autoComplete->isChecked();
    o.dragNoAsk = ui_->dragNoAsk->isChecked();
    o.addFiles = ui_->dropFiles->isChecked();
    o.addFolders = ui_->dropFolders->isChecked();
    o.recursive = ui_->dropRecursive->isChecked();
    o.addHidden = ui_->addHidden->isChecked();
    o.launchFile = ui_->launch->isChecked();
    o.naturalSort = ui_->naturalSort->isChecked();
    o.undoButton = ui_->undoButton->currentIndex();
    o.forceDir = ui_->forceDir->isChecked();
    o.detectAbsDir = ui_->detectAbs->isChecked();
    o.copy = ui_->copy->isChecked();
    o.genLog = ui_->genLog->isChecked();
    o.dstRelativeToFile = ui_->dst->isChecked();
    o.renameSidecar = ui_->sidecar->isChecked();
    o.numberDuplicates = ui_->numDup->isChecked();
    o.folderExt = ui_->folderExt->isChecked();
    o.startSwitch = ui_->startSwitch->isChecked() ? ui_->startTab->currentIndex() : -1;
    o.startClearLog = ui_->startClearLog->isChecked();
    o.finishSwitch = ui_->finishSwitch->isChecked() ? ui_->finishTab->currentIndex() : -1;
    o.finishClearFiles = ui_->finishClearFiles->isChecked();
    o.finishClearBatch = ui_->finishClearBatch->isChecked();
    o.saveLog = ui_->saveLog->isChecked();
    o.saveLogAppend = ui_->saveLogAppend->isChecked();
    o.saveLogFile = ui_->logFile->text();
    if (auto* it = ui_->languages->currentItem()) {
        QSettings s = appSettings();
        s.setValue(u"language"_s, it->data(0, Qt::UserRole).toString());
    }
    o.applyToCore();
    o.save();
    ui_.reset();
    QDialog::accept();
}

// --- About ---
AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("About this program"));
    setWindowIcon(tbIcon(TbAbout));
    resize(520, 440);
    auto* v = new QVBoxLayout(this);

    auto* head = new QHBoxLayout;
    auto* img = new QLabel;
    img->setPixmap(QPixmap(u":/antrenamer-qt.png"_s).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* title = new QLabel(u"<span style='font-size:18pt;font-weight:bold'>Ant Renamer Qt</span><br>"_s +
                             tr("Version %1").arg(displayVersion()));
    head->addWidget(img);
    head->addWidget(title, 1);
    v->addLayout(head);

    auto* tabs = new QTabWidget;
    auto browser = [](const QString& html) {
        auto* b = new QTextBrowser;
        b->setOpenExternalLinks(true);
        b->setHtml(html);
        return b;
    };
    const QString onlineHelp =
        helpLanguageCode() == u"fr"
            ? u"<a href='https://morfredus.fr/ant-renamer-qt/aide/'>morfredus.fr/ant-renamer-qt/aide</a><br>"_s
            : u"<a href='https://morfredus.fr/ant-renamer-qt-en/help/'>morfredus.fr/ant-renamer-qt-en/help</a><br>"_s;
    tabs->addTab(
        browser(u"<b>Copyright &copy; 2000-2026 Antoine Potten - Frédéric Biron</b><br><br>"_s +
                tr("Ant Renamer Qt is a C++/Qt port of Ant Renamer by Antoine Potten, developed in parallel with the original "
                   "program. Port (Windows and Linux): Frédéric Biron.") +
                u"<br><br>"_s + tr("Ant Renamer Qt support: ") +
                u"<a href='https://github.com/morfredus/ant-renamer-qt/'>github.com/morfredus/ant-renamer-qt</a> ("_s +
                tr("issues and discussions") + u")<br>"_s + tr("Support by e-mail (no GitHub account needed): ") +
                u"<a href='mailto:ant-renamer-qt@morfredus.fr'>ant-renamer-qt@morfredus.fr</a><br><br>"_s + tr("Ant Renamer Qt web site: ") +
                u"<a href='https://morfredus.fr/ant-renamer-qt/'>morfredus.fr/ant-renamer-qt</a><br>"_s + tr("Online help: ") +
                onlineHelp +
                tr("Web site of the original program (Antoine Potten): ") +
                u"<a href='https://www.antp.be/software/renamer/'>www.antp.be/software/renamer/</a><br><br>"_s + u"<b>"_s +
                tr("License:") + u"</b><br>"_s +
                tr("This program is free software; you can redistribute it and/or modify it under the terms of the GNU General "
                   "Public License as published by the Free Software Foundation; either version 2 of the License, or (at your "
                   "option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY "
                   "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
                   "<a href='https://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>GNU General Public License</a> for more "
                   "details.")),
        tr("General"));
    tabs->addTab(
        browser(
            u"<b>"_s + tr("Port:") + u"</b><br>"_s + tr("C++/Qt 6 port and Linux port: ") + u"Frédéric Biron<br><br>"_s +
            u"<b>"_s + tr("Thanks to:") + u"</b><br>Antoine Potten, "_s + tr("author of the original program") +
            u"<br>大眼仔~旭, "_s + tr("update of the Chinese (Simplified) translation") + u"<br><br><b>"_s + tr("Translations:") +
            u"</b><br>"_s +
            u"Arek Czak, Asabukuro, Carlos A Medina R, Damir13, Denny, dzmitry[li], elega, GVG, Hiro5, Ivo Blaauw, Jim Liu, "
            u"Kari Eveli, Karin Andersson, Lin Fan (林繁), N.Nedelev, Nazar Tychkivskyi, Roberto Paiva, Salvatore Meschini, "
            u"Sepp Winkler, Seunghoe Yang, Ulisses F Braga, Vico//Koby, Volker Schmidt, Wasilis Mandratzis, Zoltán Danhauser, "
            u"大眼仔~旭, 无名用户, தமிழ்நேரக்குழு<br><br><b>"_s +
            tr("Icons:") + u"</b><br>Scrows (Virtual Plastic), Windows XP (Ant Renamer)<br><br><b>"_s +
            tr("Components:") + u"</b><br>Qt (<a href='https://www.qt.io'>www.qt.io</a>)<br>"_s +
            u"<a href='https://gcc.gnu.org/'>GCC</a>, <a href='https://www.msys2.org/'>MSYS2</a>, "_s +
            u"<a href='https://cmake.org/'>CMake</a>, <a href='https://ninja-build.org/'>Ninja</a><br>"_s +
            u"<a href='https://www.jrsoftware.org/'>Inno Setup</a>, "_s +
            u"<a href='https://github.com/linuxdeploy/linuxdeploy'>linuxdeploy</a>, <a href='https://flatpak.org/'>Flatpak</a><br>"_s +
            u"<a href='https://clang.llvm.org/extra/clang-tidy/'>clang-tidy</a>, "_s +
            u"<a href='https://github.com/features/actions'>GitHub Actions</a>, <a href='https://gnupg.org/'>GnuPG</a>"_s),
        tr("Credits"));

    // history of the original program, in the interface language when available
    const QString code = helpLanguageCode();
    auto* hist = new QTextBrowser;
    hist->setOpenExternalLinks(true);
    hist->setSource(QUrl::fromLocalFile(QDir(dataPath(u"help"_s)).filePath(u"history_%1.md"_s.arg(code))));
    tabs->addTab(hist, tr("History"));
    v->addWidget(tabs, 1);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close);
    auto* upd = bb->addButton(tr("Check for updates"), QDialogButtonBox::ActionRole);
    upd->setVisible(!runsInFlatpak());
    connect(upd, &QPushButton::clicked, this, [this, upd] {
        upd->setEnabled(false);
        auto* c = new UpdateChecker(this);
        connect(c, &UpdateChecker::finished, this,
                [this, upd, c](bool ok, const QString& version, const QString& url, const QString& notes, const QString& error) {
                    c->deleteLater();
                    upd->setEnabled(true);
                    reportUpdate(this, ok, version, url, notes, error, true);
                });
        c->check(appOptions().checkPrereleases);
    });
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    v->addWidget(bb);
}
