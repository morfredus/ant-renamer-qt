#include "ren_files.h"

#include "ren_exif.h"
#include "ren_path.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace Qt::StringLiterals;

namespace ren {

static QString tr(const char* s) {
    return QCoreApplication::translate("Ren", s);
}

Options& options() {
    static Options o;
    return o;
}
std::atomic<bool>& stopFlag() {
    static std::atomic<bool> s{false};
    return s;
}

QDateTime adjustFileTime(const QDateTime& t) {
    if (!t.isValid() || options().dstRelativeToFile) {
        return t;
    }
    // Windows' old behaviour: convert with today's UTC offset instead of the one valid at that date
    const QDateTime u = t.toUTC().addSecs(QDateTime::currentDateTime().offsetFromUtc());
    return QDateTime(u.date(), u.time());
}

namespace {

bool pathExists(const QString& p) {
    return QFileInfo::exists(p) || QFileInfo(p).isSymLink();
}

#ifdef Q_OS_WIN
QString lastSysError() {
    DWORD e = GetLastError();
    wchar_t* buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, e, 0,
                   reinterpret_cast<LPWSTR>(&buf), 0, nullptr);
    QString s = buf ? QString::fromWCharArray(buf).trimmed() : QString::number(e);
    LocalFree(buf);
    return s;
}
// MoveFileW, not QFile::rename: the latter refuses case-only renames (a.txt -> A.txt)
// on case-insensitive volumes, and the Case action relies on them.
bool moveFile(const QString& from, const QString& to, QString& err) {
    QString f = QDir::toNativeSeparators(from), t = QDir::toNativeSeparators(to);
    if (MoveFileW(reinterpret_cast<LPCWSTR>(f.utf16()), reinterpret_cast<LPCWSTR>(t.utf16()))) {
        return true;
    }
    err = lastSysError();
    return false;
}
#else
bool moveFile(const QString& from, const QString& to, QString& err) {
    QFile f(from);
    if (f.rename(to)) {
        return true;
    }
    err = f.errorString();
    return false;
}
#endif

bool changedSince(const Move& m) {
    if (m.size < 0) {
        return false;
    }
    const QFileInfo fi(m.to);
    return fi.size() != m.size || fi.lastModified().toMSecsSinceEpoch() != m.mtime;
}

}  // namespace

quint64 nextId() {
    static std::atomic<quint64> n{0};
    return ++n;
}

File::File(const QString& path)
    : folder_(extractFilePath(path)), name_(extractFileName(path)), isFolder_(QFileInfo(path).isDir()) {}

bool File::exists() const {
    return pathExists(filePath());
}

qint64 File::size() const {
    return QFileInfo(filePath()).size();
}
QDateTime File::createdDate() const {
    return adjustFileTime(QFileInfo(filePath()).birthTime());
}
QDateTime File::modifiedDate() const {
    return adjustFileTime(QFileInfo(filePath()).lastModified());
}

bool File::rename(QString newName) {
    const Options& o = options();
#ifdef Q_OS_WIN
    const bool absolute = o.detectAbsDir && (newName.mid(1, 2) == u":\\" || newName.left(2) == u"\\\\");
#else
    const bool absolute = o.detectAbsDir && newName.startsWith(u'/');
#endif
    if (!absolute) {
        newName = folder_ + newName;
    }
    const QString source = filePath();
    if (newName == source) {
        description_ = tr("%1 was not renamed : %2").arg(name_, tr("new name is the same than old name"));
        status_ = State::NotRenamed;
        return false;
    }
    if (newName.contains(QChar(0))) {  // the system would cut the name at the NUL: refuse instead of renaming to something else
        description_ = tr("Could not rename %1 to %2: %3")
                           .arg(source, newName.remove(QChar(0)), tr("the new name contains an invalid character"));
        status_ = State::Error;
        return false;
    }
    if (o.forceDir) {
        QDir().mkpath(extractFilePath(newName));
    }
    QString err;
    bool ok;
    const bool moving = copyDone_ || !o.copy;
    if (moving) {
        oldName_ = source;
        ok = moveFile(source, newName, err);
    } else {
        oldName_.clear();
        QFile f(source);
        ok = f.copy(newName);
        if (ok) {
            copyDone_ = true;
        } else {
            err = f.errorString();
        }
    }
    if (ok) {
        if (onRenamed) {
            onRenamed(*this, source, newName);
        }
        name_ = extractFileName(newName);
        if (folder_ == extractFilePath(newName)) {
            description_ = tr("%1 renamed to %2").arg(source, name_);
        } else {
            description_ = tr("%1 renamed to %2").arg(source, newName);
            folder_ = extractFilePath(newName);
        }
        sidecarsMoved_.clear();
        if (o.renameSidecar && !o.copy && !isFolder_) {
            moveSidecars(source, newName, false);
        }
        status_ = State::Ok;
        if (moving && onMoved) {
            Move m{source, newName, sidecarsMoved_};
            if (!isFolder_) {  // a folder's date changes with its content: not a usable fingerprint
                const QFileInfo fi(newName);
                m.size = fi.size();
                m.mtime = fi.lastModified().toMSecsSinceEpoch();
            }
            onMoved(m);
        }
    } else {
        if (pathExists(newName)) {
            err = tr("A file or a folder with this name already exists");
        }
        description_ = tr("Could not rename %1 to %2: %3").arg(source, newName, err);
        status_ = State::Error;
        oldName_.clear();
    }
    return ok;
}

bool File::revert() {
    if (oldName_.isEmpty()) {
        description_ = tr("No previous name stored for %1 to revert old name").arg(name_);
        status_ = State::NotRenamed;
        return true;
    }
    const QString source = filePath();
    QString err;
    if (moveFile(source, oldName_, err)) {
        if (onRenamed) {
            onRenamed(*this, source, oldName_);
        }
        description_ = tr("%1 reverted to previous name %2").arg(name_, extractFileName(oldName_));
        moveSidecars(source, oldName_, true);
        status_ = State::Ok;
        name_ = extractFileName(oldName_);
        folder_ = extractFilePath(oldName_);
        oldName_.clear();
        return true;
    }
    if (pathExists(oldName_)) {
        err = tr("A file or a folder with this name already exists");
    }
    description_ = tr("Could not rename %1 to %2: %3").arg(source, oldName_, err);
    status_ = State::Error;
    return false;
}

bool File::undoMove(const QString& from, const std::vector<std::pair<QString, QString>>& sidecars) {
    oldName_ = from;
    sidecarsMoved_ = sidecars;
    return revert();
}

void File::followPath(const QString& to) {
    const QString p = QDir::toNativeSeparators(to);
    folder_ = extractFilePath(p);
    name_ = extractFileName(p);
}

// Moves the sidecars (.xmp, .dop) of `from` so they match `to` (or puts them back when `revert`). A problem with a
// sidecar never fails the picture: it is only mentioned in the log line.
void File::moveSidecars(const QString& from, const QString& to, bool revert) {
    std::vector<std::pair<QString, QString>> pairs;  // (old, new) of every sidecar to move
    if (revert) {
        for (const auto& p : sidecarsMoved_) {
            pairs.emplace_back(p.second, p.first);
        }
        sidecarsMoved_.clear();
    } else {
        const QStringList a = sidecarCandidates(from, sidecarExtensions()), b = sidecarCandidates(to, sidecarExtensions());
        for (int i = 0; i < a.size() && i < b.size(); ++i) {
            pairs.emplace_back(a[i], b[i]);
        }
    }
    for (auto [src, dst] : pairs) {
        if (src == dst || !QFileInfo::exists(src)) {
            continue;  // checked now: on Windows .xmp and .XMP are one file
        }
        if (!revert) {  // on Windows IMG.orf finds IMG.ORF: the sidecar keeps the extension it really has
            const QFileInfo si(src);
            const QStringList real = si.dir().entryList({si.fileName()}, QDir::Files);
            const QString ext = real.isEmpty() ? QString() : QFileInfo(real.first()).suffix();
            if (!ext.isEmpty() && ext != si.suffix()) {
                src = si.absolutePath() + u'/' + real.first();
                dst = dst.left(dst.size() - ext.size()) + ext;
            }
        }
        QString err;
        const bool onlyCase = src.compare(dst, Qt::CaseInsensitive) == 0;
        if (!onlyCase && pathExists(dst)) {
            err = tr("A file or a folder with this name already exists");
        } else if (moveFile(src, dst, err)) {
            if (!revert) {
                sidecarsMoved_.emplace_back(src, dst);
            }
            if (onSidecarMoved) {
                onSidecarMoved(src, dst);
            }
            description_ += u" ("_s + tr("sidecar file %1 renamed to %2").arg(extractFileName(src), extractFileName(dst)) + u')';
            continue;
        }
        description_ += u" ("_s + tr("sidecar file %1 not renamed: %2").arg(extractFileName(src), err) + u')';
    }
}

void File::fixPathAfterFolderRename(const QString& from, const QString& to) {
    if (folder_.startsWith(from, Qt::CaseInsensitive)) {
        folder_ = to + folder_.mid(from.size());
    }
    if (extractFilePath(oldName_).startsWith(from, Qt::CaseInsensitive)) {
        oldName_ = to + oldName_.mid(from.size());
    }
}

File& FileList::add(const QString& path) {
    items_.push_back(std::make_unique<File>(path));
    wire(*items_.back());
    byId_[items_.back()->id()] = items_.back().get();
    return *items_.back();
}

void FileList::wire(File& f) {
    f.onRenamed = [this](File& who, const QString& src, const QString& dst) {
        if (!who.isFolder()) {
            return;
        }
        const QString from = includeTrailingBackslash(src), to = includeTrailingBackslash(dst);
        for (auto& it : items_) {
            if (it.get() != &who) {
                it->fixPathAfterFolderRename(from, to);
            }
        }
    };
    f.onSidecarMoved = [this](const QString& from, const QString& to) {  // a .xmp that is also in the list follows
        const QString moved = QDir::fromNativeSeparators(from);          // candidates mix '/' and native separators
        for (auto& it : items_) {
            if (!it->isFolder() && QDir::fromNativeSeparators(it->filePath()).compare(moved, Qt::CaseInsensitive) == 0) {
                it->followPath(to);
            }
        }
    };
    f.onMoved = [this](const Move& m) {
        if (journal_ && journal_->recording()) {
            journal_->record(m);
        }
    };
}

File* FileList::findByPath(const QString& path) {
    const QString p = QDir::fromNativeSeparators(path);
    File* loose = nullptr;
    for (auto& it : items_) {
        const QString q = QDir::fromNativeSeparators(it->filePath());
        if (q == p) {
            return it.get();
        }
        if (!loose && q.compare(p, Qt::CaseInsensitive) == 0) {
            loose = it.get();
        }
    }
    return loose;
}

int FileList::undoPasses(int count, const std::function<void(File&)>& progress, int session) {
    if (!journal_) {
        return 0;
    }
    auto& passes = journal_->passes_;
    int done = 0;
    while (done < count && !passes.empty() && !stopFlag()) {
        if (session < 0) {
            session = passes.back().session;
        }
        const auto last = std::find_if(passes.rbegin(), passes.rend(), [session](const Pass& p) { return p.session == session; });
        if (last == passes.rend()) {
            break;
        }
        Pass& pass = *last;
        while (!pass.moves.empty() && !stopFlag()) {
            const Move& m = pass.moves.back();
            File* f = findByPath(m.to);
            std::unique_ptr<File> loose;  // no longer in the list: undone all the same, on a temporary entry
            if (!f) {
                loose = std::make_unique<File>(m.to);
                wire(*loose);
                f = loose.get();
            }
            bool ok = false;
            if (!f->exists()) {
                f->setError(tr("%1 was not renamed : %2").arg(m.to, tr("Source file not found")), State::Error);
            } else if (changedSince(m)) {  // possibly another file under the same name, put there outside the program
                f->setError(tr("%1 was not renamed : %2").arg(m.to, tr("The file was changed after the renaming")),
                            State::Error);
            } else {
                ok = f->undoMove(m.from, m.sidecars);
            }
            if (progress) {
                progress(*f);
            }
            if (!ok) {
                return done;  // the pass stays, with the moves that remain
            }
            pass.moves.pop_back();
        }
        if (!pass.moves.empty()) {
            return done;  // stopped by the user
        }
        passes.erase(std::next(last).base());
        ++done;
    }
    return done;
}

void Journal::beginPass(const QString& summary) {
    cur_ = Pass{next_, QDateTime::currentDateTime(), summary, {}, session_};
    open_ = true;
}

void Journal::record(Move m) {
    cur_.moves.push_back(std::move(m));
}

void Journal::endPass() {
    if (!open_) {
        return;
    }
    open_ = false;
    if (cur_.moves.empty()) {
        return;
    }
    ++next_;
    passes_.push_back(std::move(cur_));
    cur_ = Pass{};
    trim();
}

QByteArray Journal::toJson() const {
    QJsonArray ps;
    for (const Pass& p : passes_) {
        QJsonArray ms;
        for (const Move& m : p.moves) {
            QJsonArray sc;
            for (const auto& s : m.sidecars) {
                sc.append(QJsonArray{s.first, s.second});
            }
            ms.append(QJsonObject{{u"from"_s, m.from},
                                  {u"to"_s, m.to},
                                  {u"size"_s, m.size},
                                  {u"mtime"_s, m.mtime},
                                  {u"sidecars"_s, sc}});
        }
        ps.append(QJsonObject{{u"number"_s, p.number},
                              {u"when"_s, p.when.toString(Qt::ISODate)},
                              {u"summary"_s, p.summary},
                              {u"moves"_s, ms},
                              {u"session"_s, p.session}});
    }
    return QJsonDocument(QJsonObject{{u"version"_s, 1}, {u"next"_s, next_}, {u"passes"_s, ps}}).toJson(QJsonDocument::Compact);
}

bool Journal::fromJson(const QByteArray& data) {
    const QJsonObject root = QJsonDocument::fromJson(data).object();
    if (root.value(u"version"_s).toInt() != 1) {
        return false;
    }
    passes_.clear();
    open_ = false;
    int last = 0, lastSession = 0;
    for (const QJsonValue pv : root.value(u"passes"_s).toArray()) {
        const QJsonObject po = pv.toObject();
        Pass p;
        p.number = po.value(u"number"_s).toInt();
        p.when = QDateTime::fromString(po.value(u"when"_s).toString(), Qt::ISODate);
        p.summary = po.value(u"summary"_s).toString();
        p.session = po.value(u"session"_s).toInt();
        for (const QJsonValue mv : po.value(u"moves"_s).toArray()) {
            const QJsonObject mo = mv.toObject();
            Move m;
            m.from = mo.value(u"from"_s).toString();
            m.to = mo.value(u"to"_s).toString();
            m.size = qint64(mo.value(u"size"_s).toDouble(-1));
            m.mtime = qint64(mo.value(u"mtime"_s).toDouble());
            for (const QJsonValue sv : mo.value(u"sidecars"_s).toArray()) {
                const QJsonArray pair = sv.toArray();
                if (pair.size() == 2) {
                    m.sidecars.emplace_back(pair[0].toString(), pair[1].toString());
                }
            }
            if (!m.from.isEmpty() && !m.to.isEmpty()) {
                p.moves.push_back(std::move(m));
            }
        }
        if (!p.moves.empty()) {
            last = qMax(last, p.number);
            lastSession = qMax(lastSession, p.session);
            passes_.push_back(std::move(p));
        }
    }
    next_ = std::max({root.value(u"next"_s).toInt(), last + 1, 1});
    session_ = lastSession + 1;  // this run is a new session
    trim();
    return true;
}

void FileList::sort() {
    std::stable_sort(items_.begin(), items_.end(), [](auto& a, auto& b) { return a->position < b->position; });
}

QString exportPreview(const std::vector<const File*>& files, bool csv) {
    auto q = [](QString v) { return u'"' + v.replace(u'"', u"\"\""_s) + u'"'; };
    QString out;
    if (csv) {
        out += u"\"Folder\";\"Name\";\"New name\"\r\n"_s;  // CRLF like any CSV (RFC 4180)
    }
    for (const File* f : files) {
        const QString& to = f->preview.isEmpty() ? f->fileName() : f->preview;
        out += csv ? q(f->fileFolder()) + u';' + q(f->fileName()) + u';' + q(to) + u"\r\n"_s : to + u'\n';
    }
    return out;
}

}  // namespace ren
