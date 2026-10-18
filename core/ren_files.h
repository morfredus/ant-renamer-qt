#pragma once
// Port of Files.pas: one entry of the list to rename, plus the list itself.
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <atomic>
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
    void initBeforeJob() { preview.clear(); copyDone_ = false; }

    // After a folder rename, entries below it must follow the move.
    void fixPathAfterFolderRename(const QString& from, const QString& to);

    // The file was moved by its picture (sidecar): follow it.
    void followPath(const QString& to);

    // Set by FileList.
    std::function<void(File&, const QString&, const QString&)> onRenamed;
    std::function<void(const QString&, const QString&)> onSidecarMoved;

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

private:
    std::vector<std::unique_ptr<File>> items_;
};

}  // namespace ren
