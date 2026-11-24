#pragma once
// One entry of the list to rename, plus the list itself.
#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QTime>

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace ren {

enum class State { NotRenamed, Ok, Error, Info };

struct Error {
    QString description;
    State status = State::NotRenamed;
};

// Global processing options.
struct Options {
    bool forceDir = false;          // create missing target folders
    bool detectAbsDir = false;      // a new name with a drive/UNC prefix is an absolute path
    bool folderExt = false;         // treat folder names as having an extension
    bool copy = false;              // copy instead of rename
    bool skipHidden = true;         // files flagged `hidden` (hidden or system file) are left alone by every operation
    bool dstRelativeToFile = true;  // file times use the DST rule of the date itself (else today's offset)
    bool renameSidecar = true;      // a renamed picture takes its sidecars (IMG.xmp, IMG.CR2.xmp, IMG.dop...) along
    bool numberDuplicates = true;   // Date and time / EXIF: files given the same name are numbered _01, _02... (see numberDuplicates())
};
// Text of the core in the language of the program (context "Ren"), for the messages of the log.
QString tr(const char* s);
// Flatpak: true for a file given alone through the document portal (it cannot be renamed, see ren_files.cpp)
bool isPortalSingleFile(const QString& path);

// Identifier that is never reused during a run: what a view stores to find a File or an Action again. Unlike a pointer,
// an id whose object was removed finds nothing (FileList::byId, ActionList::byId return nullptr).
quint64 nextId();

Options& options();
QDateTime adjustFileTime(const QDateTime& t);  // applies options().dstRelativeToFile
std::atomic<bool>& stopFlag();

// One real move done during a pass: the file (or folder) went from `from` to `to`, with the sidecars moved along.
struct Move {
    QString from, to;
    std::vector<std::pair<QString, QString>> sidecars;  // (from, to) of each sidecar
    // Size and modification time (ms) of the file at `to` right after the move; the undo refuses a file that differs (a
    // renaming keeps both, so a difference means another file or an edit). size -1: not checked (folders).
    qint64 size = -1, mtime = 0;
    // true: `to` is a copy of `from` made by "Copy instead of rename" (the original stays where it is). The undo deletes the
    // copy, if it is unchanged (same size and date as right after the copy), instead of moving it back.
    bool copy = false;
    // true: nothing moved, the modification date of the file `to` (= `from`) was changed ("Set the dates from EXIF"). `mtime` is
    // the new date (the fingerprint the undo checks), `oldMtime` the one to put back.
    bool date = false;
    qint64 oldMtime = 0;
    std::vector<std::pair<QString, qint64>> sidecarDates = {};  // date moves: (sidecar, its old modification time in ms) that followed
};

// One run of the batch ("Go"): everything it moved, in order.
struct Pass {
    int number = 0;
    QDateTime when;
    QString summary;  // the actions of the batch
    std::vector<Move> moves;
    int session = 0;  // the run of the program that made it: the passes of a session are undone one after the other, sessions are independent
};

// What was renamed, pass by pass. Within a session the passes can only be undone from the last one backwards; the passes of
// another session are not affected (a file that has been renamed since is simply not found, and the undo stops).
class Journal {
public:
    void beginPass(const QString& summary);
    void endPass();  // an empty pass (nothing moved) is not kept
    void record(Move m);
    void clear() {  // the passes known so far stay in seen_: a merge does not bring them back
        passes_.clear();
        open_ = false;
    }
    bool recording() const { return open_; }
    bool hasCurrentSession() const {  // a pass made since this start of the program
        return !passes_.empty() && passes_.back().session == session_;
    }
    size_t size() const { return passes_.size(); }
    const std::deque<Pass>& passes() const { return passes_; }  // oldest first

    // The passes kept between two runs of the program (JSON). fromJson replaces the passes; false if the data is not a journal.
    QByteArray toJson() const;
    bool fromJson(const QByteArray& data);
    // Adds the passes of `data` that this journal has never seen: those another run of the program wrote meanwhile (a `-q` run
    // while the window is open). A pass this journal knew and no longer has (undone, forgotten) is not brought back. They are
    // placed by date. Returns how many were added; the Journal is left alone if `data` is not a journal.
    int mergeFromJson(const QByteArray& data);

private:
    static QString keyOf(const Pass& p);  // identifies a pass between runs: date, number, session, actions, first move
    std::set<QString> seen_;              // keys of every pass loaded, made or merged by this journal
    friend class FileList;
    void trim() {
        while (int(passes_.size()) > max_) {
            passes_.pop_front();
        }
    }
    std::deque<Pass> passes_;
    Pass cur_;
    bool open_ = false;
    int next_ = 1;
    int session_ = 1;  // session of the passes being recorded: the one after the last stored one
    static constexpr int max_ = 50;  // passes kept
};

class File {
public:
    explicit File(const QString& path);

    const QString& fileName() const { return name_; }
    const QString& fileFolder() const { return folder_; }
    QString filePath() const { return folder_ + name_; }
    bool isFolder() const { return isFolder_; }
    bool exists() const;

    quint64 id() const { return id_; }

    QString preview;
    bool hidden = false;  // hidden or system file (set by the interface, see Options::skipHidden)
    int position = 0;
    QString description() const { return description_; }
    State status() const { return status_; }
    QString oldName() const { return oldName_; }  // full path before the last move (empty after a revert)
    void setError(const QString& d, State s) {
        description_ = d;
        status_ = s;
    }

    qint64 size() const;
    QDateTime createdDate() const;
    QDateTime modifiedDate() const;

    bool rename(QString newName);
    bool revert();
    // Puts the file back to `from` after a move recorded in the journal (`sidecars` as it recorded them).
    bool undoMove(const QString& from, const std::vector<std::pair<QString, QString>>& sidecars);
    // Deletes the copy this entry stands for and points the entry back to the original `from`.
    bool undoCopy(const QString& from);
    // Gives the file the modification date `t` (the content is not touched), and the same date to its `sidecars` (a sidecar
    // that cannot be changed is only mentioned in description(), like for a renaming), and records it in the journal. `self`
    // false: the file already has the date, only the sidecars are changed. False, with the reason in description(), if the
    // system refuses the file.
    bool setModifiedDate(const QDateTime& t, bool self = true, const QStringList& sidecars = {});
    // Puts back the modification date `oldMtime` (ms) of the file and the dates of its sidecars after a move recorded with
    // Move::date.
    bool undoDate(qint64 oldMtime, const std::vector<std::pair<QString, qint64>>& sidecarDates = {});
    void initBeforeJob() {
        preview.clear();
        copyDone_ = false;
    }

    // After a folder rename, entries below it must follow the move.
    void fixPathAfterFolderRename(const QString& from, const QString& to);

    // The file was moved by its picture (sidecar): follow it.
    void followPath(const QString& to);

    // Set by FileList.
    std::function<void(File&, const QString&, const QString&)> onRenamed;
    std::function<void(const QString&, const QString&)> onSidecarMoved;
    std::function<void(const Move&)> onMoved;  // a move or a copy succeeded (Move::copy)

private:
    void moveSidecars(const QString& from, const QString& to, bool revert);
    quint64 id_ = nextId();
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
    File* byId(quint64 id) const {
        const auto it = byId_.find(id);
        return it == byId_.end() ? nullptr : it->second;
    }
    void clear() {
        items_.clear();
        byId_.clear();
    }
    void remove(size_t i) {
        byId_.erase(items_[i]->id());
        items_.erase(items_.begin() + i);
    }
    void remove(const File* f) {
        for (size_t i = 0; i < items_.size(); ++i) {
            if (items_[i].get() == f) {
                remove(i);
                return;
            }
        }
    }
    void sort();  // by position

    // The journal records every move made by the files of this list (optional, owned by the caller).
    void setJournal(Journal* j) { journal_ = j; }
    Journal* journal() const { return journal_; }
    // Undoes the last `count` passes of `session` (-1: the session of the newest pass), newest first, each one from its last
    // move to its first. Stops at the first pass that cannot be completely undone (the moves that failed stay in the
    // journal). Returns the passes fully undone.
    int undoPasses(int count, const std::function<void(File&)>& progress = {}, int session = -1);
    // "Set the dates from EXIF": the modification date of each file becomes its EXIF shooting date (see planDate). A file
    // without such a date, a folder, a hidden file (Options::skipHidden) and a file that already has the right date are left
    // alone, each with a line of explanation. One pass of the journal, so the history can undo it. Returns the files changed.
    int setDatesFromExif(const std::function<void(File&)>& progress = {});

private:
    void wire(File& f);
    File* findByPath(const QString& path);
    std::vector<std::unique_ptr<File>> items_;
    std::unordered_map<quint64, File*> byId_;
    Journal* journal_ = nullptr;
};

// The EXIF (or XMP) shooting date of a picture, local time; nullopt if the file has none. Only the DateTimeOriginal tag counts:
// DateTime is the date of the last edit, not of the shot.
std::optional<QDateTime> shootingDate(const QString& path);

// What "Set the dates from EXIF" would do to one file, without touching it.
struct DatePlan {
    // Skipped: a folder, or a hidden file while Options::skipHidden is on. Sidecar (planDates only): a sidecar of a picture
    // that is in the list too, the picture takes it along (as for a renaming, Options::renameSidecar).
    enum class Kind { Change, Same, NoDate, Skipped, Sidecar };
    Kind kind = Kind::Skipped;
    QDateTime current, shot;  // the modification date now, and the shooting date (valid for Change and Same)
    bool self = false;        // the file's own date differs (Change); else only its sidecars do
    QStringList sidecars;     // the existing sidecars whose date differs from the shooting date: they follow the picture
};
DatePlan planDate(const File& f);
// The plan of every file of the list, in order: planDate, except for the sidecars of pictures that are in the list.
std::vector<DatePlan> planDates(FileList& list);

// Which files of the list to keep (or to remove): by one of their dates (a period, and/or a time of the day) and by size.
// Every criterion that is set must hold. A folder is judged on its dates only; the size limits are for files.
struct FileFilter {
    enum class DateKind { Modified, Created, Shot };  // Shot: EXIF shooting date, else the modification date
    DateKind kind = DateKind::Modified;
    std::optional<QDateTime> from, to;        // period, both ends included
    std::optional<QTime> timeFrom, timeTo;    // time of the day, both ends included; from after to means across midnight
    std::optional<qint64> minSize, maxSize;   // bytes, both ends included
    bool isEmpty() const { return !from && !to && !timeFrom && !timeTo && !minSize && !maxSize; }
    bool matches(const File& f) const;
};

// One CSV field: between quotes, quotes doubled, and made harmless for a spreadsheet (a leading = + - @ is not a formula).
QString csvField(QString v);

// Text of the exported preview: csv = "Folder";"Name";"New name" (semicolon, every field quoted), otherwise only the new names,
// one per line. A file with no preview (or an unchanged one) keeps its current name.
QString exportPreview(const std::vector<const File*>& files, bool csv);

}  // namespace ren
