#pragma once
// Port of Files.pas: one entry of the list to rename, plus the list itself.
#include <QDateTime>
#include <QString>
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
    bool dstRelativeToFile = true;  // file times use the DST rule of the date itself (else today's offset)
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

    // Set by FileList.
    std::function<void(File&, const QString&, const QString&)> onRenamed;

private:
    QString folder_, name_, oldName_, description_;
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
