#include "dialogs.h"

#include "appsettings.h"
#include "icons.h"
#include "lngtranslator.h"
#include "shellmenu.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTreeView>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace {

QDialogButtonBox* standardButtons(QDialog* d) {
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(bb, &QDialogButtonBox::accepted, d, &QDialog::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, d, &QDialog::reject);
    return bb;
}

}  // namespace

// --- Add folders ------------------------------------------------------------------------------------
AddFoldersDialog::AddFoldersDialog(const QString& startFolder, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Select folders to add"));
    setWindowIcon(tbIcon(TbAddFolder));
    resize(450, 520);
    auto* v = new QVBoxLayout(this);

    model_ = new QFileSystemModel(this);
    model_->setFilter(QDir::Dirs | QDir::Drives | QDir::NoDotAndDotDot);
    model_->setRootPath(QString());
    view_ = new QTreeView;
    view_->setModel(model_);
    view_->setRootIndex(model_->index(QString()));
    for (int c = 1; c < model_->columnCount(); ++c) view_->hideColumn(c);
    view_->setHeaderHidden(true);
    view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    v->addWidget(view_, 1);

    files_ = new QCheckBox(tr("Add files of selected folders"));
    folders_ = new QCheckBox(tr("Add selected folders"));
    recursive_ = new QCheckBox(tr("Apply these two options also to subfolders"));
    const AppOptions& o = appOptions();
    files_->setChecked(o.addFiles);
    folders_->setChecked(o.addFolders);
    recursive_->setChecked(o.recursive);
    v->addWidget(files_);
    v->addWidget(folders_);
    v->addWidget(recursive_);
    v->addWidget(standardButtons(this));

    const QModelIndex start = model_->index(startFolder);
    if (start.isValid()) {
        view_->setCurrentIndex(start);
        view_->scrollTo(start);
    }
}

QStringList AddFoldersDialog::folders() const {
    QStringList l;
    for (const QModelIndex& i : view_->selectionModel()->selectedRows(0)) l << model_->filePath(i);
    return l;
}

AddChoice AddFoldersDialog::choice() const { return {files_->isChecked(), folders_->isChecked(), recursive_->isChecked()}; }

// --- Drag & drop --------------------------------------------------------------------------------------
DragDropDialog::DragDropDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Drag & Drop Options"));
    setWindowIcon(tbIcon(TbAddFiles));
    auto* v = new QVBoxLayout(this);
    v->addWidget(new QLabel(tr("Options to use to add files that have been dropped:")));
    files_ = new QCheckBox(tr("Add files of selected folders"));
    folders_ = new QCheckBox(tr("Add selected folders"));
    recursive_ = new QCheckBox(tr("Apply these two options also to subfolders"));
    const AppOptions& o = appOptions();
    files_->setChecked(o.addFiles);
    folders_->setChecked(o.addFolders);
    recursive_->setChecked(o.recursive);
    notAgain_ = new QCheckBox(tr("Do not show this window again (keep selected options for next times)"));
    for (auto* c : {files_, folders_, recursive_}) v->addWidget(c);
    v->addSpacing(8);
    v->addWidget(notAgain_);
    v->addWidget(standardButtons(this));
}

AddChoice DragDropDialog::choice() const { return {files_->isChecked(), folders_->isChecked(), recursive_->isChecked()}; }
bool DragDropDialog::dontAskAgain() const { return notAgain_->isChecked(); }

// --- Options ---------------------------------------------------------------------------------------------
struct OptionsDialog::Ui {
    QCheckBox *realtime, *resizeCols, *icons, *forceFont, *autoComplete, *dragNoAsk, *launch, *naturalSort, *newUi, *forceDir, *detectAbs,
        *copy, *genLog, *folderExt, *sidecar, *dst, *portable, *shellMenu;
    QCheckBox *dropFiles, *dropFolders, *dropRecursive;
    QComboBox* pageLayout = nullptr;
    QSpinBox *historyMax, *fontSize;
    QFontComboBox* font;
    QComboBox* iconSet;
    QCheckBox *startSwitch, *startClearLog, *finishSwitch, *finishClearFiles, *finishClearBatch, *saveLog, *saveLogAppend;
    QComboBox *startTab, *finishTab;
    QLineEdit* logFile;
    QTreeWidget* languages;
    QLabel *langVersion, *langAuthors, *langComments;
};

OptionsDialog::OptionsDialog(QWidget* parent) : QDialog(parent), ui_(new Ui) {
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

    {   // Display
        auto* l = page(tr("Display"));
        header(l, tr("Lists"));
        ui_->realtime = box(l, tr("Real-time update when renaming files"), o.realtime);
        ui_->resizeCols = box(l, tr("Auto-resize columns of Files list"), o.resizeCols);
        ui_->icons = box(l, tr("Show icons in files list, based on file extension"), o.fileIcons);
        ui_->newUi = box(l, tr("Modern interface layout (used at the next start)"), o.newUi);
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

        header(l, tr("Fonts"));
        ui_->forceFont = box(l, tr("Force the following font for controls that have to display special characters:"), o.forceFont);
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
        l->addStretch();
    }
    {   // Files & folders
        auto* l = page(tr("Files && Folders"));
        header(l, tr("Drag & Drop"));
        ui_->dragNoAsk = box(l, tr("Do not ask what to do, always use the following options:"), o.dragNoAsk);
        ui_->dropFiles = box(l, tr("Add files of selected folders"), o.addFiles);
        ui_->dropFolders = box(l, tr("Add selected folders"), o.addFolders);
        ui_->dropRecursive = box(l, tr("Apply these two options also to subfolders"), o.recursive);
        for (auto* c : {ui_->dropFiles, ui_->dropFolders, ui_->dropRecursive}) c->setContentsMargins(20, 0, 0, 0);
        header(l, tr("List behaviour"));
        ui_->launch = box(l, tr("Launch file on double click"), o.launchFile);
        ui_->naturalSort = box(l, tr("Natural sort in files list (2 before 10) instead of alphabetical"), o.naturalSort);
        header(l, tr("Windows Explorer"));
        ui_->shellMenu = box(l, tr("Add \"Add to Ant Renamer Qt\" to the context menu of files and folders"), shellMenuInstalled());
#ifdef Q_OS_WIN
        l->addWidget(new QLabel(tr("On Windows 11 the entry is in \"Show more options\" (or Shift+F10).")));
#else
        ui_->shellMenu->setEnabled(false);
#endif
        l->addStretch();
    }
    {   // Processing
        auto* l = page(tr("Processing"));
        header(l, tr("File renaming"));
        ui_->copy = box(l, tr("Copy instead of Rename"), o.copy);
        ui_->genLog = box(l, tr("Generate log"), o.genLog);
        ui_->dst = box(l, tr("DST offset related to file date"), o.dstRelativeToFile);
        ui_->sidecar = box(l, tr("Also rename the sidecar files (.xmp, .dop) of a renamed picture"), o.renameSidecar);
        header(l, tr("Folder rules"));
        ui_->forceDir = box(l, tr("Force directories (allows to create folders if \"\\\" are found in names)"), o.forceDir);
        ui_->detectAbs = box(l, tr("Detect a \":\\\" or \"\\\\\" as an absolute path (allows to move files to anywhere)"), o.detectAbsDir);
        ui_->folderExt = box(l, tr("Consider the last \".\" in a folder name as extension delimiter, like for files"), o.folderExt);
        l->addStretch();
    }
    {   // Events
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
            const QString f = QFileDialog::getSaveFileName(this, tr("Save log"), ui_->logFile->text(), tr("Text files (*.log *.txt)"));
            if (!f.isEmpty()) ui_->logFile->setText(QDir::toNativeSeparators(f));
        });
        r3->addWidget(ui_->saveLog);
        r3->addWidget(ui_->logFile, 1);
        r3->addWidget(browse);
        l->addLayout(r3);
        ui_->saveLogAppend = box(l, tr("Append to current file"), o.saveLogAppend);
        l->addStretch();
    }
    {   // Language: the original program's .lng files, then where the settings live
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
            if (li.file == cur) ui_->languages->setCurrentItem(item);
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
        l->addWidget(new QLabel(tr("The new language is used the next time the program starts.")));

        header(l, tr("Settings storage"));
        ui_->portable = box(l, tr("Portable mode: keep the settings in the program folder"), isPortable());
    }
    v->addWidget(standardButtons(this));
}

void OptionsDialog::accept() {
    // storage first, so everything below is written where it will be read from next time
    if (ui_->portable->isChecked() != isPortable() && !setPortable(ui_->portable->isChecked()))
        QMessageBox::warning(this, windowTitle(), tr("Could not write the settings in the program folder."));

    if (ui_->shellMenu->isChecked() != shellMenuInstalled() && !setShellMenu(ui_->shellMenu->isChecked(), tr("Add to Ant Renamer Qt")))
        QMessageBox::warning(this, windowTitle(), tr("Could not change the Explorer context menu."));

    AppOptions& o = appOptions();
    if (o.newUi != ui_->newUi->isChecked() || (ui_->newUi->isChecked() && o.pageLayout != ui_->pageLayout->currentIndex()))
        QMessageBox::information(this, windowTitle(), tr("The new interface is used the next time the program starts."));
    o.newUi = ui_->newUi->isChecked();
    o.pageLayout = ui_->pageLayout->currentIndex();
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
    o.launchFile = ui_->launch->isChecked();
    o.naturalSort = ui_->naturalSort->isChecked();
    o.forceDir = ui_->forceDir->isChecked();
    o.detectAbsDir = ui_->detectAbs->isChecked();
    o.copy = ui_->copy->isChecked();
    o.genLog = ui_->genLog->isChecked();
    o.dstRelativeToFile = ui_->dst->isChecked();
    o.renameSidecar = ui_->sidecar->isChecked();
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
    delete ui_;
    ui_ = nullptr;
    QDialog::accept();
}

// --- About ---------------------------------------------------------------------------------------------------
AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("About this program"));
    setWindowIcon(tbIcon(TbAbout));
    resize(520, 440);
    auto* v = new QVBoxLayout(this);

    auto* head = new QHBoxLayout;
    auto* img = new QLabel;
    img->setPixmap(QPixmap(u":/ant.bmp"_s));
    auto* title = new QLabel(u"<span style='font-size:18pt;font-weight:bold'>Ant Renamer Qt</span><br>"_s +
                             tr("Version %1").arg(QApplication::applicationVersion()));
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
    tabs->addTab(browser(
        u"<b>Copyright &copy; 2000-2026 Antoine Potten - Frédéric Biron</b><br><br>"_s +
        tr("Ant Renamer Qt is a C++/Qt port of Ant Renamer by Antoine Potten, developed in parallel with the original program. Port (Windows and Linux): Frédéric Biron.") + u"<br><br>"_s +
        tr("Ant Renamer Qt support: ") + u"<a href='https://github.com/morfredus/ant-renamer-qt/'>github.com/morfredus/ant-renamer-qt</a> ("_s + tr("issues and discussions") + u")<br><br>"_s +
        tr("Web site address: ") + u"<a href='https://www.antp.be/software/renamer/'>www.antp.be/software/renamer/</a><br><br>"_s +
        u"<b>"_s + tr("License:") + u"</b><br>"_s +
        tr("This program is free software; you can redistribute it and/or modify it under the terms of the GNU General "
           "Public License as published by the Free Software Foundation; either version 2 of the License, or (at your "
           "option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY "
           "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
           "<a href='https://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>GNU General Public License</a> for more details.")),
        tr("General"));
    tabs->addTab(browser(
        u"<b>"_s + tr("Port:") + u"</b><br>"_s + tr("C++/Qt 6 port and Linux port: ") + u"Frédéric Biron<br><br>"_s +
        u"<b>"_s + tr("Thanks to:") + u"</b><br>Sebastien Buysse, Grégory Zicot, Michel Potten, "_s +
        tr("...and all the people that sent me suggestions and comments") + u"<br><br><b>"_s + tr("Translations:") + u"</b><br>"_s +
        u"Arek Czak, Asabukuro, GVG, Hiro5, Ivo Blaauw, Jim Liu, Kari Eveli, Karin Andersson, Lin Fan, N.Nedelev, "
        u"Nazar Tychkivskyi, Roberto Paiva, Salvatore Meschini, Sepp Winkler, Seunghoe Yang<br><br><b>"_s +
        tr("Components:") + u"</b><br>Qt (<a href='https://www.qt.io'>www.qt.io</a>)"_s),
        tr("Credits"));

    // version history of the original program, in the language of the interface when available
    const QString code = helpLanguageCode();
    auto* hist = new QTextBrowser;
    hist->setOpenExternalLinks(true);
    hist->setSource(QUrl::fromLocalFile(QDir(dataPath(u"help"_s)).filePath(u"history_%1.html"_s.arg(code))));
    tabs->addTab(hist, tr("History"));
    v->addWidget(tabs, 1);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    v->addWidget(bb);
}
