#pragma once
#include <QDialog>
#include <QStringList>

#include <memory>

class QCheckBox;
class QFileSystemModel;
class QLineEdit;
class QTreeView;

struct AddChoice {
    bool files = true, folders = false, recursive = true, hidden = false;  // hidden: also hidden and system files
    bool any() const { return files || folders; }
};

// Remembers the "what to add" choice in the options (the check boxes below start from it). Not saved to disk.
void rememberAddChoice(const AddChoice& c);

// The four "what to add" check boxes shared by the Add folders and Drag & drop dialogs, preset from the options.
struct AddChoiceBoxes {
    AddChoiceBoxes();
    AddChoice choice() const;
    QCheckBox *files, *folders, *recursive, *hidden;
};

// "Add folders": pick one or more folders and what to take from them (fSelectDirectory.pas).
class AddFoldersDialog : public QDialog {
public:
    AddFoldersDialog(const QString& startFolder, QWidget* parent);
    QStringList folders() const;
    AddChoice choice() const;

private:
    QTreeView* view_;
    QFileSystemModel* model_;
    QLineEdit* path_;
    AddChoiceBoxes boxes_;
};

// Shown when files are dropped on the list (fDragdropOptions.pas).
class DragDropDialog : public QDialog {
public:
    explicit DragDropDialog(QWidget* parent);
    AddChoice choice() const;
    bool dontAskAgain() const;

private:
    AddChoiceBoxes boxes_;
    QCheckBox* notAgain_;
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
