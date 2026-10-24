#pragma once
#include "actionpanel.h"
#include "dialogs.h"
#include "ren_actions.h"
#include "ren_files.h"

#include <QMainWindow>
#include <QPersistentModelIndex>
#include <QThread>
#include <QTreeWidget>
#include <vector>

class QAction;
class QFrame;
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
    QString folder, name, preview;  // the row as the worker left it: the GUI never reads a File that is being modified
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
    RenWorker(ren::ActionList* a, ren::FileList* f, ren::Op op, int undoCount = 1) : actions_(a), files_(f), op_(op), undoCount_(undoCount) {}
    ren::Op op() const { return op_; }
signals:
    void progress(const Snapshot& s);

protected:
    void run() override;

private:
    ren::ActionList* actions_;
    ren::FileList* files_;
    ren::Op op_;
    int undoCount_;
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
    void resizeEvent(QResizeEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;

private:
    enum Col { ColFolder, ColFileName, ColFullPath, ColExt, ColPreview, ColSize, ColCreated, ColModified, ColCount };

    // construction
    QAction* makeAction(const QString& text, const QString& hint, int icon, const QKeySequence& key = {});
    void createActions();
    void createToolbars();
    // Modern layout, "single screen": the three pages (bar + content) side by side in splitters instead of tabs.
    void buildSingleScreen();
    void arrangeSingle(int mode);   // 1 bands, 2 columns
    void saveSingleState();
    void setPaneVisible(int page, bool on);
    int pageLayout_ = 0;            // option: 0 tabs, 1 bands, 2 columns
    bool singleScreen_ = false;
    int singleMode_ = 1;            // arrangement shown now (columns give way to bands in a narrow window)
    QWidget* panes_[3] = {};
    bool paneVisible_[3] = {true, true, true};
    QWidget* singleHost_ = nullptr;
    QSplitter *outer_ = nullptr, *inner_ = nullptr;
    QByteArray stBands_, stColsOuter_, stColsInner_;
    void updateTabCounts();
    bool newUi_ = false;  // modern layout (option, read at start)
    void createPages();
    void createStatusBar();
    void loadSettings();
    void saveSettings();
    void applyOptionsLive();
    void startUpdateCheck();
    class UpdateChecker* updater_ = nullptr;
    int theme_ = -1;  // theme in use (see theme.h)

    // navigation / status
    void setStatus();
    void setStatus(int percent, const QString& text);

    // files
    void addFiles(const QStringList& paths, const AddChoice* choice);
    void exportPreview();
    void addFolderContents(const QString& dir, const AddChoice& c);
    void addFile(const QString& path);
    void filesDropped(const QStringList& paths);
    void importDropped(const QStringList& paths);
    void updateItem(QTreeWidgetItem* item);
    void updateAllItems();
    ren::File* fileOf(QTreeWidgetItem* item) const;
    void renumberFiles();
    void removeSelectedFiles();
    void removeAllFiles();
    void removeDeadFiles();
    void fileHeaderMenu(const QPoint& pos);
    void sortFilesBy(int column);
    int sortColumn_ = -1;
    Qt::SortOrder sortOrder_ = Qt::AscendingOrder;
    void unsortFiles();

    // batch
    void addToBatch();
    void addBatchItem(ren::Action* a);
    void removeSelectedActions();
    void editBatchItem(QTreeWidgetItem* item);  // load an action of the plan into the editor
    void updateBatchItem();                     // replace the selected action with the editor settings
    void applyEditorTo(QTreeWidgetItem* item);
    void validateEdit();                        // "Validate the modification" of the editor zone
    void endEdit();
    QFrame* editBar_ = nullptr;                 // modern layout: shown under the editor while an action is edited
    QLabel* editLabel_ = nullptr;
    QPersistentModelIndex editIndex_;           // the plan row being edited (invalid once it is removed)
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
    void run(ren::Op op, int undoCount = 1);
    void showHistory();
    void updateUndoState();
    void onProgress(const Snapshot& s);
    void onFinished();
    void setBusy(bool busy);  // disables what would modify the lists, the batch or the options while a run is in progress

    // log
    void addLog(const QString& text, ren::State st);
    void rebuildLog();
    void logAction(int which);

    ren::Journal journal_;  // the passes of this session, for Undo and the history window
    ren::FileList files_;
    ren::ActionList batch_;
    RenWorker* worker_ = nullptr;
    bool clearBatchAfter_ = false;
    bool exitAfterRun_ = false;
    QHash<quintptr, QTreeWidgetItem*> rows_;

    struct LogEntry { QString text; ren::State state; };
    std::vector<LogEntry> logEntries_;
    size_t runLogStart_ = 0;  // first log entry of the current run (the auto-saved log holds this run only)
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

    QAction *aGo_, *aStop_, *aUndo_, *aHistory_, *aOnTop_, *aTheme_, *aOptions_, *aHelp_, *aAbout_, *aExit_;
    QAction *aTab_[3];
    QAction *aAddFiles_, *aAddFolders_, *aRemoveSel_, *aRemoveAll_, *aRemoveDead_, *aPreview_, *aChangedOnly_, *aShowHidden_;
    QAction *aBatchAdd_, *aBatchEdit_, *aBatchRemoveSel_, *aBatchClear_, *aBatchOpen_, *aBatchSave_;
    QAction *aLogFilter_, *aLogOk_, *aLogError_, *aLogNot_, *aLogClear_, *aLogSave_, *aLogCopy_;
    QAction *aUp_, *aDown_, *aTop_, *aBottom_;
    QAction *aSelAll_, *aSelNone_, *aSelInvert_, *aUnsort_;
};
