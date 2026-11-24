#pragma once
#include <QDialog>
#include <QStringList>

#include <memory>

#include "ren_files.h"

class QCheckBox;
class QFileSystemModel;
class QLineEdit;
class QListWidget;
class QTreeView;

struct AddChoice {
    bool files = true, folders = false, recursive = true, hidden = false;  // hidden: also hidden and system files
    bool any() const { return files || folders; }
};

// Version shown to the user; a pre-release also shows its build commit. The real version stays
// QApplication::applicationVersion().
QString displayVersion();

// Remembers the "what to add" choice in the options (not saved to disk).
void rememberAddChoice(const AddChoice& c);

// The "what to add" check boxes shared by the Add folders and Drag & drop dialogs.
struct AddChoiceBoxes {
    AddChoiceBoxes();
    AddChoice choice() const;
    QCheckBox *files, *folders, *recursive, *hidden;
};

// "Add folders": pick folders and what to take from them.
class AddFoldersDialog : public QDialog {
public:
    AddFoldersDialog(const QString& startFolder, QWidget* parent);
    QStringList folders() const;
    AddChoice choice() const;

private:
    bool portal_;  // Flatpak: folders come one by one from the portal picker
    QListWidget* view_ = nullptr;  // portal mode: the chosen folders
    QStringList folders_;
    QTreeView* tree_ = nullptr;  // normal mode: the file system tree
    QFileSystemModel* model_ = nullptr;
    QLineEdit* path_ = nullptr;
    AddChoiceBoxes boxes_;
};

// Shown when files are dropped on the list.
class DragDropDialog : public QDialog {
public:
    explicit DragDropDialog(QWidget* parent);
    AddChoice choice() const;
    bool dontAskAgain() const;

private:
    AddChoiceBoxes boxes_;
    QCheckBox* notAgain_;
};

// "Filter files": keep (or remove) the files of the list by date, time of the day and size.
class FilterDialog : public QDialog {
public:
    // `previous` and `keep` fill the fields again (to correct the last filter); removed > 0 adds the choice to put back the
    // files that the previous filter took out of the list before filtering.
    explicit FilterDialog(QWidget* parent, const ren::FileFilter& previous = {}, bool keep = true, int removed = 0);
    ~FilterDialog() override;
    ren::FileFilter filter() const;
    bool keepMatching() const;  // true: the files that match stay; false: they are removed
    bool putBack() const;       // put the files of the previous filter back before filtering
    bool clearRequested() const;  // the "Clear the filter" button: put every file back, filter nothing

private:
    struct Ui;
    std::unique_ptr<Ui> ui_;
};

class OptionsDialog : public QDialog {
public:
    explicit OptionsDialog(QWidget* parent);
    ~OptionsDialog() override;
    void accept() override;

private:
    struct Ui;
    std::unique_ptr<Ui> ui_;
};

class AboutDialog : public QDialog {
public:
    explicit AboutDialog(QWidget* parent);
};
