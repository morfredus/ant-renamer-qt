#pragma once
#include "actionpanel.h"
#include "dialogs.h"
#include "ren_actions.h"
#include "ren_files.h"

#include <QMainWindow>
#include <QThread>
#include <QTreeWidget>
#include <vector>

class QAction;
class QGroupBox;
class QLabel;
class QListWidget;
class QProgressBar;
class QSplitter;
class QStackedWidget;
class QToolBar;

// What the worker tells the GUI after each file (copied, so the GUI never reads a File being modified).
struct Snapshot {
    quintptr file = 0;
    QString description;
    int state = 0;
    int batchPos = 0, index = 0;
};
Q_DECLARE_METATYPE(Snapshot)

// A flat list that can be reordered by drag and drop; optionally accepts files dropped from Explorer.
class ItemTree : public QTreeWidget {
    Q_OBJECT
public:
    explicit ItemTree(bool acceptUrls);
signals:
    void urlsDropped(const QStringList& paths);
    void reordered();

protected:
    void dragEnterEvent(QDragEnterEvent*) override;
    void dragMoveEvent(QDragMoveEvent*) override;
    void dropEvent(QDropEvent*) override;

private:
    bool acceptUrls_;
};

// Runs the batch off the GUI thread, as the original TRenThread did.
class RenWorker : public QThread {
    Q_OBJECT
public:
    RenWorker(ren::ActionList* a, ren::FileList* f, ren::Op op) : actions_(a), files_(f), op_(op) {}
    ren::Op op() const { return op_; }
signals:
    void progress(const Snapshot& s);

protected:
    void run() override;

private:
    ren::ActionList* actions_;
    ren::FileList* files_;
    ren::Op op_;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    void processCommandLine(const QStringList& args);
    void showTab(int tab);
    void addShellPaths(const QStringList& paths);  // from the Explorer context menu

protected:
    void closeEvent(QCloseEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;

private:
    enum Col { ColFolder, ColFileName, ColFullPath, ColExt, ColPreview, ColSize, ColCreated, ColModified, ColCount };

    // construction
    QAction* makeAction(const QString& text, const QString& hint, int icon, const QKeySequence& key = {});
    void createActions();
    void createToolbars();
    void createPages();
    void createStatusBar();
    void loadSettings();
    void saveSettings();
    void applyOptionsLive();

    // navigation / status
    void setStatus();
    void setStatus(int percent, const QString& text);

    // files
    void addFiles(const QStringList& paths, const AddChoice* choice);
    void addFolderContents(const QString& dir, const AddChoice& c);
    void addFile(const QString& path);
    void filesDropped(const QStringList& paths);
    void updateItem(QTreeWidgetItem* item);
    void updateAllItems();
    ren::File* fileOf(QTreeWidgetItem* item) const;
    void renumberFiles();
    void removeSelectedFiles();
    void removeAllFiles();
    void removeDeadFiles();
    void fileHeaderMenu(const QPoint& pos);
    void sortFilesBy(int column);
    void unsortFiles();

    // batch
    void addToBatch();
    void addBatchItem(ren::Action* a);
    void removeSelectedActions();
    void clearBatch();
    void renumberActions();
    void renumberBatchView();
    void openBatch();
    void openBatchFile(const QString& path);
    void saveBatch();
    void updateRecentMenu();
    void setBatchVisible(bool on);

    // generic list helpers
    void moveItems(int mode);
    QTreeWidget* currentList() const;

    // running
    void updateSamplePreview();
    void run(ren::Op op);
    void onProgress(const Snapshot& s);
    void onFinished();

    // log
    void addLog(const QString& text, ren::State st);
    void rebuildLog();
    void logAction(int which);

    ren::FileList files_;
    ren::ActionList batch_;
    RenWorker* worker_ = nullptr;
    bool clearBatchAfter_ = false;
    bool exitAfterRun_ = false;
    QHash<quintptr, QTreeWidgetItem*> rows_;

    struct LogEntry { QString text; ren::State state; };
    std::vector<LogEntry> logEntries_;
    bool logShowOk_ = true, logShowError_ = true, logShowNot_ = true;

    // widgets
    QStackedWidget* stack_;
    ItemTree* fileView_;
    ItemTree* batchView_;
    ActionPanel* panel_;
    QGroupBox *previewGroup_, *batchGroup_;
    QLabel *previewOld_, *previewNew_;
    QSplitter* actionsSplit_;
    QListWidget* logView_;
    QProgressBar* progress_;
    QLabel *countLabel_, *statusLabel_;
    QToolBar *tbProgram_, *tbTabs_, *tbFiles_, *tbActions_, *tbLog_, *tbMove_;
    QMenu* recentMenu_;
    int currentTab_ = 0;

    QAction *aGo_, *aStop_, *aUndo_, *aOnTop_, *aOptions_, *aHelp_, *aAbout_, *aExit_;
    QAction *aTab_[3];
    QAction *aAddFiles_, *aAddFolders_, *aRemoveSel_, *aRemoveAll_, *aRemoveDead_, *aPreview_;
    QAction *aBatchAdd_, *aBatchRemoveSel_, *aBatchClear_, *aBatchOpen_, *aBatchSave_;
    QAction *aLogFilter_, *aLogOk_, *aLogError_, *aLogNot_, *aLogClear_, *aLogSave_, *aLogCopy_;
    QAction *aUp_, *aDown_, *aTop_, *aBottom_;
    QAction *aSelAll_, *aSelNone_, *aSelInvert_, *aUnsort_;
};
