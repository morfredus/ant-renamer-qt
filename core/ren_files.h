#pragma once
// Port of Files.pas: one entry of the list to rename, plus the list itself.
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

namespace ren {

enum class State { NotRenamed, Ok, Error, Info };

struct Error {
    QString description;
    State status = State::NotRenamed;
};

// Replaces the Delphi GlobalSettings / GlobalStop globals.
struct Options {
    bool forceDir = false;      // create missing target folders
    bool detectAbsDir = false;  // a new name with a drive/UNC prefix is an absolute path
    bool folderExt = false;     // treat folder names as having an extension
    bool copy = false;          // copy instead of rename
    bool skipHidden = true;     // files flagged `hidden` (hidden or system file) are left alone by every operation
    bool dstRelativeToFile = true;  // file times use the DST rule of the date itself (else today's offset)
    bool renameSidecar = true;  // a renamed picture takes its sidecars (IMG.xmp, IMG.CR2.xmp, IMG.dop...) along
};
Options& options();
QDateTime adjustFileTime(const QDateTime& t);  // applies options().dstRelativeToFile
std::atomic<bool>& stopFlag();

// One real move done during a pass: the file (or folder) went from `from` to `to`, with the sidecars moved along.
struct Move {
    QString from, to;
    std::vector<std::pair<QString, QString>> sidecars;  // (from, to) of each sidecar
};

// One run of the batch ("Go"): everything it moved, in order.
struct Pass {
    int number = 0;
    QDateTime when;
    QString summary;  // the actions of the batch
    std::vector<Move> moves;
};

// What this session renamed, pass by pass. Passes can only be undone from the last one backwards.
class Journal {
public:
    void beginPass(const QString& summary);
    void endPass();  // an empty pass (nothing moved) is not kept
    void record(Move m);
    void clear() { passes_.clear(); open_ = false; }
    void setMaxPasses(int n) { max_ = n < 1 ? 1 : n; trim(); }
    bool recording() const { return open_; }
    size_t size() const { return passes_.size(); }
    const std::deque<Pass>& passes() const { return passes_; }  // oldest first

private:
    friend class FileList;
    void trim() { while (int(passes_.size()) > max_) passes_.pop_front(); }
    std::deque<Pass> passes_;
    Pass cur_;
    bool open_ = false;
    int next_ = 1;
    int max_ = 50;
};

class File {
public:
    explicit File(const QString& path);

    const QString& fileName() const { return name_; }
    const QString& fileFolder() const { return folder_; }
    QString filePath() const { return folder_ + name_; }
    bool isFolder() const { return isFolder_; }
    bool exists() const;

    QString preview;
    bool hidden = false;  // hidden or system file (set by the interface, see Options::skipHidden)
    int position = 0;
    QString description() const { return description_; }
    State status() const { return status_; }
    void setError(const QString& d, State s) { description_ = d; status_ = s; }

    qint64 size() const;
    QDateTime createdDate() const;
    QDateTime modifiedDate() const;

    bool rename(QString newName);
    bool revert();
    // Puts the file back to `from` after a move recorded in the journal (`sidecars` as it recorded them).
    bool undoMove(const QString& from, const std::vector<std::pair<QString, QString>>& sidecars);
    void initBeforeJob() { preview.clear(); copyDone_ = false; }

    // After a folder rename, entries below it must follow the move.
    void fixPathAfterFolderRename(const QString& from, const QString& to);

    // The file was moved by its picture (sidecar): follow it.
    void followPath(const QString& to);

    // Set by FileList.
    std::function<void(File&, const QString&, const QString&)> onRenamed;
    std::function<void(const QString&, const QString&)> onSidecarMoved;
    std::function<void(const Move&)> onMoved;  // a real move succeeded (not a copy)

private:
    void moveSidecars(const QString& from, const QString& to, bool revert);
    QString folder_, name_, oldName_, description_;
    std::vector<std::pair<QString, QString>> sidecarsMoved_;  // (from, to) of the last rename, to put them back on revert
    State status_ = State::NotRenamed;
    bool isFolder_ = false;
    bool copyDone_ = false;
};

class FileList {
public:
    File& add(const QString& path);
    size_t size() const { return items_.size(); }
    File& at(size_t i) { return *items_[i]; }
    void clear() { items_.clear(); }
    void remove(size_t i) { items_.erase(items_.begin() + i); }
    void remove(const File* f) {
        for (size_t i = 0; i < items_.size(); ++i)
            if (items_[i].get() == f) { items_.erase(items_.begin() + i); return; }
    }
    void sort();  // by position

    // The journal records every move made by the files of this list (optional, owned by the caller).
    void setJournal(Journal* j) { journal_ = j; }
    Journal* journal() const { return journal_; }
    // Undoes the last `count` passes, newest first, each one from its last move to its first. Stops at the first pass
    // that cannot be completely undone (the moves that failed stay in the journal). Returns the passes fully undone.
    int undoPasses(int count, const std::function<void(File&)>& progress = {});

private:
    void wire(File& f);
    File* findByPath(const QString& path);
    std::vector<std::unique_ptr<File>> items_;
    Journal* journal_ = nullptr;
};

}  // namespace ren
