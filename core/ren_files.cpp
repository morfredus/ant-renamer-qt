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
#include <QSet>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace Qt::StringLiterals;

namespace ren {

QString tr(const char* s) {
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

// Flatpak: a file given alone to the program (dropped from the file manager or chosen with the file picker) is
// /run/user/<uid>/doc/<id>/<name>: exactly two components after "doc". The portal then "renames" it by writing a hidden
// temporary copy (.xdp-...) and deleting the original, and the last step fails: the file is lost to the user. A rename of
// such a file is refused before anything is touched. A folder given to the program is /doc/<id>/<folder>/...: deeper, it works.
bool isPortalSingleFile(const QString& path) {
    static const QString root = QStringLiteral("/run/user/");
    if (!path.startsWith(root)) {
        return false;
    }
    const QStringList parts = path.mid(root.size()).split(QLatin1Char('/'), Qt::SkipEmptyParts);  // uid, "doc", id, name
    return parts.size() == 4 && parts.at(1) == QLatin1String("doc");
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
    if (isPortalSingleFile(from) || isPortalSingleFile(to)) {
        err = QCoreApplication::translate("RenFiles", "file given alone to the Flatpak sandbox (drag and drop, file picker): it cannot be renamed, nothing was changed. Add the folder that contains it instead (Add folders, or drag the folder)");
        return false;
    }
    QFile f(from);
    if (f.rename(to)) {
        return true;
    }
    err = f.errorString();
    return false;
}
#endif

// Sets the modification date of a file, nothing else. On Windows the handle only asks for the right to write attributes (not the
// content) and shares everything: a read-only file, or one another program keeps open (a photo editor, the Explorer preview),
// still gets its date. On the other systems the owner can set it through a read-only handle.
bool setFileMTime(const QString& path, const QDateTime& t, QString& err) {
#ifdef Q_OS_WIN
    const QString native = QDir::toNativeSeparators(path);
    const HANDLE h = CreateFileW(reinterpret_cast<LPCWSTR>(native.utf16()), FILE_WRITE_ATTRIBUTES,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                 FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        err = lastSysError();
        return false;
    }
    const quint64 ticks = quint64(t.toMSecsSinceEpoch() + 11644473600000LL) * 10000ULL;  // 100 ns since 1601
    FILETIME ft;
    ft.dwLowDateTime = DWORD(ticks & 0xffffffffULL);
    ft.dwHighDateTime = DWORD(ticks >> 32);
    const bool ok = SetFileTime(h, nullptr, nullptr, &ft) != 0;
    if (!ok) {
        err = lastSysError();
    }
    CloseHandle(h);
    return ok;
#else
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        err = f.errorString();
        return false;
    }
    const bool ok = f.setFileTime(t, QFileDevice::FileModificationTime);
    if (!ok) {
        err = f.errorString();
    }
    return ok;
#endif
}

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
        if (onMoved) {
            Move m{source, newName, sidecarsMoved_};
            m.copy = !moving;
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

bool File::undoCopy(const QString& from) {
    const QString copy = filePath();
    if (!QFile::remove(copy)) {
        description_ = tr("Could not delete the copy %1").arg(copy);
        status_ = State::Error;
        return false;
    }
    description_ = tr("Copy %1 deleted").arg(copy);
    status_ = State::Ok;
    oldName_.clear();
    sidecarsMoved_.clear();
    followPath(from);  // the entry is the original again
    return true;
}

bool File::setModifiedDate(const QDateTime& t, bool self, const QStringList& sidecars) {
    const QString path = filePath();
    const QString fmt = u"yyyy-MM-dd HH:mm:ss"_s;
    const qint64 oldMs = QFileInfo(path).lastModified().toMSecsSinceEpoch();
    QString err;
    if (self && !setFileMTime(path, t, err)) {
        description_ = tr("Could not change the date of %1: %2").arg(path, err);
        status_ = State::Error;
        return false;
    }
    Move m{path, path, {}};
    m.date = true;
    m.oldMtime = oldMs;
    QStringList problems;
    for (const QString& sc : sidecars) {  // a problem with a sidecar never fails the picture
        const qint64 old = QFileInfo(sc).lastModified().toMSecsSinceEpoch();
        QString e;
        if (setFileMTime(sc, t, e)) {
            m.sidecarDates.emplace_back(sc, old);
        } else {
            problems << tr("Could not change the date of %1: %2").arg(sc, e);
        }
    }
    description_ = self ? tr("%1: modification date changed from %2 to %3")
                              .arg(path, QDateTime::fromMSecsSinceEpoch(oldMs).toString(fmt), t.toString(fmt))
                        : tr("%1: the modification date already is %2").arg(path, t.toString(fmt));
    if (!m.sidecarDates.empty()) {
        description_ += u' ' + tr("(%1 sidecar file(s) given the same date)").arg(m.sidecarDates.size());
    }
    for (const QString& p : std::as_const(problems)) {
        description_ += u" ; "_s + p;
    }
    status_ = !self && m.sidecarDates.empty() ? State::Error : State::Ok;
    if (status_ == State::Ok && onMoved) {
        const QFileInfo after(path);
        m.size = after.size();
        m.mtime = after.lastModified().toMSecsSinceEpoch();
        onMoved(m);
    }
    return status_ == State::Ok;
}

bool File::undoDate(qint64 oldMtime, const std::vector<std::pair<QString, qint64>>& sidecarDates) {
    const QString path = filePath();
    QString err;
    if (!setFileMTime(path, QDateTime::fromMSecsSinceEpoch(oldMtime), err)) {
        description_ = tr("Could not change the date of %1: %2").arg(path, err);
        status_ = State::Error;
        return false;
    }
    description_ = tr("%1: modification date put back to %2")
                       .arg(path, QDateTime::fromMSecsSinceEpoch(oldMtime).toString(u"yyyy-MM-dd HH:mm:ss"_s));
    for (const auto& sc : sidecarDates) {  // a sidecar that is gone or refuses is only mentioned
        QString e;
        if (QFileInfo::exists(sc.first) && !setFileMTime(sc.first, QDateTime::fromMSecsSinceEpoch(sc.second), e)) {
            description_ += u" ; "_s + tr("Could not change the date of %1: %2").arg(sc.first, e);
        }
    }
    status_ = State::Ok;
    return true;
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
                ok = m.date ? f->undoDate(m.oldMtime, m.sidecarDates) : m.copy ? f->undoCopy(m.from) : f->undoMove(m.from, m.sidecars);
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

int FileList::setDatesFromExif(const std::function<void(File&)>& progress) {
    if (journal_) {
        journal_->beginPass(tr("Set the modification date from the EXIF shooting date"));
    }
    int changed = 0;
    const std::vector<DatePlan> plans = planDates(*this);
    for (size_t i = 0; i < items_.size(); ++i) {
        if (stopFlag()) {
            break;
        }
        File& f = *items_[i];
        const DatePlan& p = plans[i];
        const QString path = f.filePath();
        switch (p.kind) {
        case DatePlan::Kind::Change:
            if (f.setModifiedDate(p.shot, p.self, p.sidecars)) {
                ++changed;
            }
            break;
        case DatePlan::Kind::Same:
            f.setError(tr("%1 was not changed : %2").arg(path, tr("its modification date already is the EXIF shooting date")),
                       State::NotRenamed);
            break;
        case DatePlan::Kind::NoDate:
            f.setError(tr("%1 was not changed : %2").arg(path, tr("no EXIF shooting date")), State::Info);
            break;
        case DatePlan::Kind::Sidecar:
            f.setError(tr("%1 was not changed : %2").arg(path, tr("follows its picture")), State::Info);
            break;
        case DatePlan::Kind::Skipped:
            f.setError(tr("%1 was not changed : %2").arg(path, f.isFolder() ? tr("a folder has no EXIF date") : tr("hidden file")),
                       State::Info);
            break;
        }
        if (progress) {
            progress(f);
        }
    }
    if (journal_) {
        journal_->endPass();
    }
    return changed;
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
    seen_.insert(keyOf(cur_));
    passes_.push_back(std::move(cur_));
    cur_ = Pass{};
    trim();
}

QByteArray Journal::toJson() const {
    QJsonArray ps;
    for (const Pass& p : passes_) {
        QJsonArray ms;
        for (const Move& m : p.moves) {
            QJsonArray sd;
            for (const auto& d : m.sidecarDates) {
                sd.append(QJsonArray{d.first, double(d.second)});
            }
            QJsonArray sc;
            for (const auto& s : m.sidecars) {
                sc.append(QJsonArray{s.first, s.second});
            }
            ms.append(QJsonObject{{u"from"_s, m.from},
                                  {u"to"_s, m.to},
                                  {u"size"_s, m.size},
                                  {u"mtime"_s, m.mtime},
                                  {u"copy"_s, m.copy},
                                  {u"date"_s, m.date},
                                  {u"oldmtime"_s, m.oldMtime},
                                  {u"sidecarDates"_s, sd},
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
    seen_.clear();
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
            m.copy = mo.value(u"copy"_s).toBool();
            m.date = mo.value(u"date"_s).toBool();
            m.oldMtime = qint64(mo.value(u"oldmtime"_s).toDouble());
            for (const QJsonValue dv : mo.value(u"sidecarDates"_s).toArray()) {
                const QJsonArray pair = dv.toArray();
                if (pair.size() == 2) {
                    m.sidecarDates.emplace_back(pair[0].toString(), qint64(pair[1].toDouble()));
                }
            }
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
    for (const Pass& p : passes_) {
        seen_.insert(keyOf(p));
    }
    return true;
}

QString Journal::keyOf(const Pass& p) {
    return p.when.toString(Qt::ISODate) + u'|' + QString::number(p.number) + u'|' + QString::number(p.session) + u'|' + p.summary +
           u'|' + (p.moves.empty() ? QString() : p.moves.front().from + u'>' + p.moves.front().to);
}

int Journal::mergeFromJson(const QByteArray& data) {
    Journal other;
    if (!other.fromJson(data)) {
        return 0;
    }
    int added = 0;
    for (Pass& p : other.passes_) {
        if (!seen_.insert(keyOf(p)).second) {
            continue;
        }
        const auto at = std::find_if(passes_.begin(), passes_.end(),
                                     [&p](const Pass& q) { return q.when.toSecsSinceEpoch() > p.when.toSecsSinceEpoch(); });
        next_ = std::max(next_, p.number + 1);
        passes_.insert(at, std::move(p));
        ++added;
    }
    trim();
    return added;
}

std::optional<QDateTime> shootingDate(const QString& path) {
    QMap<QString, QString> t;
    if (!readExif(path, t)) {
        return std::nullopt;
    }
    const QString v = t.value(u"DateTimeOriginal"_s).trimmed();
    QDateTime d = QDateTime::fromString(v, u"yyyy:MM:dd HH:mm:ss"_s);
    if (!d.isValid()) {  // XMP writes ISO 8601, with or without a zone
        d = QDateTime::fromString(v, Qt::ISODate);
        if (d.isValid() && d.timeSpec() != Qt::LocalTime) {
            d = d.toLocalTime();
        }
    }
    if (!d.isValid() || d.date().year() < 1970) {  // 0000:00:00 00:00:00 and the like: a camera without a clock
        return std::nullopt;
    }
    return d;
}

DatePlan planDate(const File& f) {
    DatePlan p;
    if (f.isFolder() || (options().skipHidden && f.hidden)) {
        return p;
    }
    p.current = QFileInfo(f.filePath()).lastModified();
    const auto shot = shootingDate(f.filePath());
    if (!shot) {
        p.kind = DatePlan::Kind::NoDate;
        return p;
    }
    p.shot = *shot;
    p.self = p.current.toSecsSinceEpoch() != p.shot.toSecsSinceEpoch();
    if (options().renameSidecar) {  // the sidecars follow their picture, like for a renaming
        // the sidecars of the picture, and those of its RAW (IMG.CR2.xmp): the RAW is itself a sidecar of the picture
        QStringList cands = sidecarCandidates(f.filePath(), sidecarExtensions());
        for (const QString& c : QStringList(cands)) {
            if (rawExtensions().contains(QFileInfo(c).suffix(), Qt::CaseInsensitive) && QFileInfo(c).isFile()) {
                cands += sidecarCandidates(c, sidecarExtensions());
            }
        }
        QSet<QString> seen;
        for (const QString& c : std::as_const(cands)) {
            const QFileInfo ci(c);
#ifdef Q_OS_WIN
            const QString key = ci.canonicalFilePath().toLower();  // "a.xmp" and "a.XMP" are one file (canonicalFilePath keeps the case given)
#else
            const QString key = ci.canonicalFilePath();
#endif
            if (ci.isFile() && !seen.contains(key)) {
                seen.insert(key);
                if (ci.lastModified().toSecsSinceEpoch() != p.shot.toSecsSinceEpoch()) {
                    p.sidecars << c;
                }
            }
        }
    }
    p.kind = p.self || !p.sidecars.isEmpty() ? DatePlan::Kind::Change : DatePlan::Kind::Same;
    return p;
}

std::vector<DatePlan> planDates(FileList& list) {
    QSet<QString> followers;  // the sidecars of the pictures of the list (names compared without case, as listedPicture does)
    if (options().renameSidecar) {
        for (size_t i = 0; i < list.size(); ++i) {
            const File& g = list.at(i);
            if (!g.isFolder()) {
                for (const QString& c : sidecarCandidates(g.filePath(), sidecarExtensions())) {
                    followers.insert(QDir::fromNativeSeparators(c).toLower());
                }
            }
        }
    }
    std::vector<DatePlan> plans;
    for (size_t i = 0; i < list.size(); ++i) {
        const File& f = list.at(i);
        if (!f.isFolder() && followers.contains(QDir::fromNativeSeparators(f.filePath()).toLower())) {
            DatePlan p;
            p.kind = DatePlan::Kind::Sidecar;
            plans.push_back(p);
        } else {
            plans.push_back(planDate(f));
        }
    }
    return plans;
}

// The date a filter looks at; invalid if the file has none (a creation date is not kept by every file system).
static QDateTime filterDate(const File& f, FileFilter::DateKind kind) {
    if (kind == FileFilter::DateKind::Created) {
        return f.createdDate();
    }
    if (kind == FileFilter::DateKind::Shot && !f.isFolder()) {
        QMap<QString, QString> t;
        if (readExif(f.filePath(), t)) {
            QString v = t.value(u"DateTimeOriginal"_s);
            if (v.isEmpty()) {
                v = t.value(u"DateTime"_s);
            }
            const QDateTime d = QDateTime::fromString(v, u"yyyy:MM:dd HH:mm:ss"_s);
            if (d.isValid()) {
                return d;
            }
        }
    }
    return f.modifiedDate();
}

bool FileFilter::matches(const File& f) const {
    if (from || to || timeFrom || timeTo) {
        const QDateTime d = filterDate(f, kind);
        if (!d.isValid()) {
            return false;
        }
        if ((from && d < *from) || (to && d > *to)) {
            return false;
        }
        if (timeFrom || timeTo) {
            const QTime t = d.time();
            const QTime a = timeFrom.value_or(QTime(0, 0, 0)), b = timeTo.value_or(QTime(23, 59, 59, 999));
            if (a <= b ? (t < a || t > b) : (t < a && t > b)) {  // across midnight: outside means between the two
                return false;
            }
        }
    }
    if ((minSize || maxSize) && !f.isFolder()) {
        const qint64 s = f.size();
        if ((minSize && s < *minSize) || (maxSize && s > *maxSize)) {
            return false;
        }
    }
    return true;
}

void FileList::sort() {
    std::stable_sort(items_.begin(), items_.end(), [](auto& a, auto& b) { return a->position < b->position; });
}

QString csvField(QString v) {
    // A name starting with = + - @ (or a tab / CR) would be run as a formula by a spreadsheet, quotes or not: a quote
    // before it makes it plain text.
    if (!v.isEmpty() && QStringView(u"=+-@\t\r").contains(v[0])) {
        v.prepend(u'\'');
    }
    return u'"' + v.replace(u'"', u"\"\""_s) + u'"';
}

QString exportPreview(const std::vector<const File*>& files, bool csv) {
    auto q = csvField;
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
