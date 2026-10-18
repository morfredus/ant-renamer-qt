#pragma once
#include <QDialog>
#include <QStringList>

class QCheckBox;
class QFileSystemModel;
class QTreeView;

struct AddChoice {
    bool files = true, folders = false, recursive = true, hidden = false;  // hidden: also hidden and system files
    bool any() const { return files || folders; }
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
    QCheckBox *files_, *folders_, *recursive_, *hidden_;
};

// Shown when files are dropped on the list (fDragdropOptions.pas).
class DragDropDialog : public QDialog {
public:
    explicit DragDropDialog(QWidget* parent);
    AddChoice choice() const;
    bool dontAskAgain() const;

private:
    QCheckBox *files_, *folders_, *recursive_, *hidden_, *notAgain_;
};

class OptionsDialog : public QDialog {
public:
    explicit OptionsDialog(QWidget* parent);
    void accept() override;

private:
    struct Ui;
    Ui* ui_;
};

class AboutDialog : public QDialog {
public:
    explicit AboutDialog(QWidget* parent);
};
