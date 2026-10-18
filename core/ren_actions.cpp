#include "ren_actions.h"

#include "ren_exif.h"
#include "ren_format.h"
#include "ren_path.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QRegularExpression>
#include <QThread>
#include <QUuid>
#include <QXmlStreamReader>
#include <algorithm>
#include <climits>

using namespace Qt::StringLiterals;

namespace ren {

namespace {

QString tr(const char* s) { return QCoreApplication::translate("Ren", s); }

// --- Pascal string semantics (1-based, forgiving about out-of-range) -------------------
QString pasCopy(const QString& s, int idx, int count = INT_MAX) {
    if (idx < 1) idx = 1;
    if (count <= 0 || idx > s.size()) return {};
    return s.mid(idx - 1, count);
}
void pasDelete(QString& s, int idx, int count) {
    if (idx < 1 || idx > s.size() || count <= 0) return;
    s.remove(idx - 1, count);
}
void pasInsert(const QString& what, QString& s, int idx) {
    s.insert(qBound(1, idx, int(s.size()) + 1) - 1, what);
}

qint64 tickCount() { return QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF; }

// Characters that cannot appear in a Windows file name; empty stays empty. NUL is dropped (the system would cut the
// name there and the file would lose its extension) and the other control characters become a dash.
QString validateFileName(QString s) {
    s.remove(QChar(0));
    for (QChar& c : s) {
        if (c.unicode() < 0x20 || c.unicode() == 0x7F) c = u'-';
        else if (c == u'"') c = u'\'';
        else if (c == u'?') c = u'.';
        else if (c == u'\\' || c == u':' || c == u'/' || c == u'*' || c == u'<' || c == u'>' || c == u'|') c = u'-';
    }
    return s;
}

QString uniqueFileName(const QString& path) {
    if (!QFileInfo::exists(path)) return path;
    QString r;
    do {
        QThread::msleep(QRandomGenerator::global()->bounded(1, 25));
        r = changeFileExt(path, QString()) + QString::number(tickCount()) + extractFileExt(path);
    } while (QFileInfo::exists(r));
    return r;
}

int countFolderContents(const QString& folder, bool countFolders, bool countFiles, bool recursive) {
    int n = 0;
    const auto list = QDir(folder).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QFileInfo& fi : list) {
        if (fi.isDir()) {
            if (countFolders) ++n;
            if (recursive) n += countFolderContents(fi.absoluteFilePath() + u'/', countFolders, countFiles, true);
        } else if (countFiles) {
            ++n;
        }
    }
    return n;
}

// The name of `fn` with `ext` (without its dot) as extension; no dot at all when `ext` is empty.
QString withExt(const QString& fn, const QString& ext) {
    return changeFileExt(extractFileName(fn), ext.isEmpty() ? QString() : u'.' + ext);
}

QString padNum(int v, int digits) {  // Delphi '%.Nd'
    QString s = QString::number(qAbs(qint64(v)));
    while (s.size() < digits) s.prepend(u'0');
    return v < 0 ? u'-' + s : s;
}

// Delphi UpperCase only maps ASCII; the "locale" option switches to full Unicode mapping.
QChar mapCase(QChar c, bool upper, bool unicode) {
    if (!unicode && c.unicode() > 127) return c;
    return upper ? c.toUpper() : c.toLower();
}
QString mapCase(const QString& s, bool upper, bool unicode) {
    QString r = s;
    for (QChar& c : r) c = mapCase(c, upper, unicode);
    return r;
}

// Same defaults as TRegExpr: case-insensitive, '.' matches newlines.
QRegularExpression makeRegex(const QString& e) {
    return QRegularExpression(e, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
}

QString expandTemplate(const QString& t, const QRegularExpressionMatch& m) {
    QString out;
    for (int i = 0; i < t.size(); ++i) {
        const QChar c = t[i];
        if (c == u'$' && i + 1 < t.size()) {
            const QChar n = t[i + 1];
            if (n == u'&') { out += m.captured(0); ++i; continue; }
            if (n.isDigit()) { out += m.captured(n.digitValue()); ++i; continue; }
            if (n == u'{') {
                int e = t.indexOf(u'}', i + 2);
                bool ok = false;
                int g = e > 0 ? t.mid(i + 2, e - i - 2).toInt(&ok) : 0;
                if (ok) { out += m.captured(g); i = e; continue; }
            }
        } else if (c == u'\\' && i + 1 < t.size()) {
            const QChar n = t[i + 1];
            out += n == u'n' ? QChar(u'\n') : n == u't' ? QChar(u'\t') : n;
            ++i;
            continue;
        }
        out += c;
    }
    return out;
}

// Attribute helper for save().
struct A {
    const char* k;
    QString v;
    A(const char* k, const QString& v) : k(k), v(v) {}
    A(const char* k, bool b) : k(k), v(b ? u"True"_s : u"False"_s) {}
    A(const char* k, int n) : k(k), v(QString::number(n)) {}
};
void begin(QXmlStreamWriter& w, const char* name, std::initializer_list<A> attrs) {
    w.writeStartElement(QString::fromLatin1(name));
    for (const A& a : attrs) w.writeAttribute(QString::fromLatin1(a.k), a.v);
}
void cdataChild(QXmlStreamWriter& w, const char* name, const QString& text) {
    w.writeStartElement(QString::fromLatin1(name));
    w.writeCDATA(text);
    w.writeEndElement();
}

QStringList splitLines(const QString& s) {
    QStringList l = s.split(QRegularExpression(u"\r\n|\n|\r"_s));
    if (!l.isEmpty() && l.last().isEmpty()) l.removeLast();  // TStringList.Text drops the trailing empty line
    return l;
}

}  // namespace

// --- XmlElem ----------------------------------------------------------------------------
int XmlElem::num(const QString& k, int def) const {
    bool ok = false;
    int v = attrs.value(k).toInt(&ok);
    return ok ? v : def;
}
bool XmlElem::flag(const QString& k, bool def) const {
    if (!attrs.contains(k)) return def;
    const QString v = attrs.value(k).trimmed().toLower();
    return v == u"true" || v == u"1" || v == u"-1" || v == u"yes";
}

// --- Action (base) ------------------------------------------------------------------------
QString Action::description() const { return tr("Do nothing"); }

bool Action::getTag(const QString& tag, QString& value) {
    value.clear();
    if (tag == u"name") {
        value = changeFileExt(extractFileName(fileName_), QString());
        return true;
    }
    if (tag == u"ext") {
        if (!plainFolder()) value = extractFileExt(fileName_);
        return true;
    }
    if (tag.startsWith(u"folder")) {
        QString cur = fileName_;
        const QString nStr = tag.mid(6);
        int n = nStr == u"N" ? 1 : nStr.toInt();
        if (n < 0) {
#ifdef Q_OS_WIN
            if (cur.startsWith(u"\\\\")) cur.remove(0, 2);
#else
            if (cur.startsWith(u'/')) cur.remove(0, 1);
#endif
            for (; n < 0; ++n) {  // Split(cur, sep, value, cur): value = before first separator, cur = rest
                int p = cur.indexOf(kSep);
                value = p < 0 ? QString() : cur.left(p);
                cur = cur.mid(p + 1);
            }
            value.remove(u':');
        } else {
            for (; n > 0; --n) cur = excludeTrailingBackslash(extractFilePath(cur));
            value = extractFileName(cur);
        }
        return true;
    }
    if (tag.startsWith(u"count")) {
        value = QString::number(countFolderContents(includeTrailingBackslash(fileName_), !tag.startsWith(u"countfiles"),
                                                    !tag.startsWith(u"countfolders"), tag.endsWith(u'r')));
        return true;
    }
    return false;
}

QString Action::makeNewName(const QString& fileName, bool isFolder) {
    fileName_ = fileName;
    isFolder_ = isFolder;
    return {};
}

void Action::setInfo(const QString& what) {
    lastError_.description = tr("%1 was not renamed : %2").arg(fileName_, what);
    lastError_.status = State::Info;
}

// A sidecar (.xmp, .dop) whose picture is in the list too: the picture renames it, so it must not be renamed on its own
// (whatever their order in the list, and whether it already followed or not).
static bool isListedSidecar(FileList& list, const File& f) {
    if (f.isFolder() || !options().renameSidecar || options().copy) return false;
    const QString me = QDir::fromNativeSeparators(f.filePath());
    for (size_t i = 0; i < list.size(); ++i) {
        const File& g = list.at(i);
        if (&g == &f || g.isFolder()) continue;
        for (const QString& c : sidecarCandidates(g.filePath(), sidecarExtensions()))
            if (QDir::fromNativeSeparators(c).compare(me, Qt::CaseInsensitive) == 0) return true;
    }
    return false;
}

void Action::perform(FileList& list, Op op, const Progress& progress) {
    lastError_ = Error{};
    if (op != Op::Revert) init();
    for (size_t i = 0; i < list.size(); ++i) {
        if (stopFlag()) break;
        File& f = list.at(i);
        curFile_ = &f;
        if (options().skipHidden && f.hidden) continue;
        if (op == Op::Rename && isListedSidecar(list, f)) {  // renamed with its picture
            if (progress) progress(f, position, int(i));
            continue;
        }
        if (op != Op::Preview) {
            QString newName;
            if (!f.exists()) {
                lastError_.description = tr("%1 was not renamed : %2").arg(f.filePath(), tr("Source file not found"));
                lastError_.status = State::Error;
            } else if (op == Op::Rename) {
                newName = makeNewName(f.filePath(), f.isFolder());
            }
            if (lastError_.status != State::NotRenamed) {
                f.setError(lastError_.description, State::NotRenamed);
                lastError_ = Error{};
            } else if (op == Op::Rename) {
                f.rename(newName);
            } else {
                f.revert();
            }
        } else {
            f.setError(QString(), State::NotRenamed);
            if (f.preview.isEmpty()) f.preview = f.fileName();
            f.preview = makeNewName(f.fileFolder() + f.preview, f.isFolder());
            lastError_ = Error{};  // previews never report action errors
        }
        if (progress) progress(f, position, int(i));
    }
    curFile_ = nullptr;
    finish();
}

// --- ChangeExt ----------------------------------------------------------------------------
QString ActionChangeExt::description() const {
    if (noReplace)
        return newExtension.isEmpty() ? tr("Do nothing") : tr("Append extension \"%1\"").arg(newExtension);
    return newExtension.isEmpty() ? tr("Remove extension") : tr("Change extension to \"%1\"").arg(newExtension);
}
QString ActionChangeExt::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString r = extractFileName(fn);
    if (plainFolder()) return r;
    return noReplace ? r + newExtension : changeFileExt(r, newExtension);
}
void ActionChangeExt::save(QXmlStreamWriter& w) const {
    begin(w, "ChangeExt", {{"NewExt", newExtension}, {"NoReplace", noReplace}});
    w.writeEndElement();
}
void ActionChangeExt::load(const XmlElem& e) {
    newExtension = e.str("NewExt", "ext");
    noReplace = e.flag("NoReplace", false);
}

// --- StringRepl ---------------------------------------------------------------------------
QString ActionStringRepl::description() const {
    QString r = tr("Replace \"%1\" by \"%2\"").arg(search, replace);
    if (all) r += tr(", replace all occurences");
    if (caseSens) r += tr(", case-sensitive");
    if (wildcards) r += tr(", with wildcards");
    if (onlyExt) r += tr(", apply to extension instead of name");
    else if (inclExt) r += tr(", also apply to extension");
    return r;
}
QString ActionStringRepl::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    const auto cs = caseSens ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QString oldName;
    if (onlyExt) {
        if (plainFolder()) return extractFileName(fn);
        oldName = extractFileExt(fn).mid(1);
    } else if (inclExt || plainFolder()) {
        oldName = extractFileName(fn);
    } else {
        oldName = changeFileExt(extractFileName(fn), QString());
    }
    QString r = oldName;
    if (!search.isEmpty() && wildcards) {
        // '*' and '?' are wildcards, everything else is literal; the replacement stays literal.
        QString pat;
        for (const QChar c : search)
            pat += c == u'*' ? QStringLiteral(".*?") : c == u'?' ? QStringLiteral(".") : QRegularExpression::escape(c);
        QRegularExpression re(pat, QRegularExpression::DotMatchesEverythingOption |
                                       (caseSens ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption));
        QString out;
        qsizetype last = 0;
        for (auto it = re.globalMatch(r); it.hasNext();) {
            const auto m = it.next();
            if (m.capturedLength() == 0) continue;  // "*" alone would match the empty string everywhere
            out += r.mid(last, m.capturedStart() - last) + replace;
            last = m.capturedEnd();
            if (!all) break;
        }
        r = out + r.mid(last);
    } else if (!search.isEmpty()) {
        if (all) r.replace(search, replace, cs);
        else if (int p = r.indexOf(search, 0, cs); p >= 0) r.replace(p, search.size(), replace);
    }
    if (onlyExt) return withExt(fn, r);
    if (!inclExt && !plainFolder()) r += extractFileExt(fn);
    return r;
}
void ActionStringRepl::save(QXmlStreamWriter& w) const {
    begin(w, "StrRepl", {{"Search", search}, {"Repl", replace}, {"AllOccurences", all},
                         {"CaseSensitive", caseSens}, {"IncludeExt", inclExt}, {"OnlyExt", onlyExt},
                         {"Wildcards", wildcards}});
    w.writeEndElement();
}
void ActionStringRepl::load(const XmlElem& e) {
    search = e.str("Search", "old");
    replace = e.str("Repl", "new");
    all = e.flag("AllOccurences", true);
    caseSens = e.flag("CaseSensitive", true);
    inclExt = e.flag("IncludeExt", false);
    onlyExt = e.flag("OnlyExt", false);
    wildcards = e.flag("Wildcards", false);
}

// --- MultstrRepl --------------------------------------------------------------------------
QString ActionMultstrRepl::description() const {
    QString r = tr("Replace multiple strings using set \"%1\"").arg(setName);
    if (includeExt) r += tr(", also apply to extension");
    if (caseSens) r += tr(", case-sensitive");
    return r;
}
QString ActionMultstrRepl::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString r = extractFileName(fn);
    const bool withExt = includeExt || plainFolder();
    if (!withExt) r = changeFileExt(r, QString());
    const auto cs = caseSens ? Qt::CaseSensitive : Qt::CaseInsensitive;
    for (const QString& line : splitLines(setChars)) {
        int tab = line.indexOf(u'\t');
        const QString src = tab < 0 ? QString() : line.left(tab);
        if (src.isEmpty()) continue;
        r.replace(src, line.mid(tab + 1), cs);
    }
    if (!withExt) r += extractFileExt(fn);
    return r;
}
void ActionMultstrRepl::save(QXmlStreamWriter& w) const {
    begin(w, "MultstrRepl", {{"SetName", setName}, {"IncludeExt", includeExt}, {"CaseSensitive", caseSens}});
    cdataChild(w, "SetChars", setChars);
    w.writeEndElement();
}
void ActionMultstrRepl::load(const XmlElem& e) {
    setName = e.str("SetName", QString());
    includeExt = e.flag("IncludeExt", false);
    caseSens = e.flag("CaseSensitive", true);
    setChars = e.childText.value("SetChars");
}

// --- StringIns ----------------------------------------------------------------------------
QString ActionStringIns::description() const {
    QString r = tr("Insert \"%1\" at position %2").arg(newString).arg(pos);
    r += fromBegin ? tr(", from begin") : tr(", from end");
    if (ext) r += tr(", apply to extension instead of name");
    return r;
}
QString ActionStringIns::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString r;
    if (!plainFolder()) {
        r = ext ? extractFileExt(fn).mid(1) : changeFileExt(extractFileName(fn), QString());
    } else {
        r = extractFileName(fn);
        if (ext) return r;
    }
    const int insPos = fromBegin ? pos + 1 : int(r.size()) - pos + 1;
    pasInsert(advancedFormat(newString, [this](auto& t, auto& v) { return getTag(t, v); }), r, insPos);
    if (!plainFolder())
        return ext ? withExt(fn, r) : r + extractFileExt(fn);
    return r;
}
void ActionStringIns::save(QXmlStreamWriter& w) const {
    begin(w, "StrIns", {{"Str", newString}, {"Pos", pos}, {"FromBegin", fromBegin}, {"Ext", ext}});
    w.writeEndElement();
}
void ActionStringIns::load(const XmlElem& e) {
    newString = e.str("Str", "new");
    pos = qBound(0, e.num("Pos", 0), 1000);
    fromBegin = e.flag("FromBegin", true);
    ext = e.flag("Ext", false);
}

// --- MoveStr ------------------------------------------------------------------------------
QString ActionMoveStr::description() const {
    return tr("Move %1 characters from position %2%3, to position %4%5")
        .arg(count).arg(fromPos).arg(fromBegin ? tr(", from begin") : tr(", from end"))
        .arg(toPos).arg(toBegin ? tr(", from begin") : tr(", from end"));
}
QString ActionMoveStr::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString r = plainFolder() ? extractFileName(fn) : changeFileExt(extractFileName(fn), QString());
    int real = fromBegin ? fromPos + 1 : int(r.size()) - fromPos - count + 1;
    const QString moved = pasCopy(r, real, count);
    pasDelete(r, real, count);
    real = toBegin ? toPos + 1 : int(r.size()) - toPos + 1;
    pasInsert(moved, r, real);
    return plainFolder() ? r : r + extractFileExt(fn);
}
void ActionMoveStr::save(QXmlStreamWriter& w) const {
    begin(w, "MoveStr", {{"FromPos", fromPos}, {"FromBegin", fromBegin}, {"Count", count}, {"ToPos", toPos}, {"ToBegin", toBegin}});
    w.writeEndElement();
}
void ActionMoveStr::load(const XmlElem& e) {
    fromPos = qBound(0, e.num("FromPos", 0), 1000);
    fromBegin = e.flag("FromBegin", true);
    count = qBound(1, e.num("Count", 1), 1000);
    toPos = qBound(0, e.num("ToPos", 0), 1000);
    toBegin = e.flag("ToBegin", true);
}

// --- CharDel ------------------------------------------------------------------------------
QString ActionCharDel::description() const {
    QString r = tr("Delete %1 characters").arg(nbChar);
    switch (afterString) {
    case 1: r += tr(" after the string \"%1\"").arg(str); break;
    case 2: r += tr(" before the string \"%1\"").arg(str); break;
    default:
        r += tr(" at position %1").arg(pos);
        r += fromBegin ? tr(", from begin") : tr(", from end");
    }
    if (ext) r += tr(", apply to extension instead of name");
    return r;
}
QString ActionCharDel::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString r;
    if (!plainFolder()) {
        r = ext ? extractFileExt(fn).mid(1) : changeFileExt(extractFileName(fn), QString());
    } else {
        r = extractFileName(fn);
        if (ext) return r;
    }
    const int found = r.indexOf(str) + 1;  // 1-based like Pos(); 0 = not found
    switch (afterString) {
    case 1:
        if (!str.isEmpty() && found > 0) pasDelete(r, found + int(str.size()), nbChar);
        break;
    case 2:
        if (!str.isEmpty() && found > 0) {
            int delPos = found - nbChar, delLen = nbChar;
            if (delPos < 1) { delLen = nbChar - 1 + delPos; delPos = 1; }
            pasDelete(r, delPos, delLen);
        }
        break;
    default:
        pasDelete(r, fromBegin ? pos + 1 : int(r.size()) - pos - nbChar + 1, nbChar);
    }
    if (!plainFolder())
        return ext ? withExt(fn, r) : r + extractFileExt(fn);
    return r;
}
void ActionCharDel::save(QXmlStreamWriter& w) const {
    begin(w, "CharDel", {{"Nb", nbChar}, {"AfterString", afterString}, {"Pos", pos},
                         {"FromBegin", fromBegin}, {"Str", str}, {"Ext", ext}});
    w.writeEndElement();
}
void ActionCharDel::load(const XmlElem& e) {
    nbChar = qBound(1, e.num("Nb", 1), 1000);
    afterString = qBound(0, e.num("AfterString", 0), 2);
    pos = qBound(0, e.num("Pos", 0), 1000);
    fromBegin = e.flag("FromBegin", true);
    str = e.str("Str", "str");
    ext = e.flag("Ext", false);
}

// --- Enum ---------------------------------------------------------------------------------
bool PairGroup::continues(const QString& fn) {
    const QFileInfo fi(fn);
    const QString k = (fi.absolutePath() + u'/' + fi.completeBaseName()).toLower();
    const QString e = fi.suffix().toLower();
    if (k == key && !exts.contains(e)) { exts << e; return true; }
    key = k;
    exts = {e};
    return false;
}
QString ActionEnum::description() const {
    return tr("Generate an enumeration with \"%1\" from %2 to %3 by %4").arg(mask).arg(startAt).arg(digits).arg(increment);
}
bool ActionEnum::getTag(const QString& tag, QString& value) {
    if (tag == u"num") { value = padNum(curPos_, digits); return true; }
    return Action::getTag(tag, value);
}
QString ActionEnum::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    if (restart) {
        const QString cur = extractFilePath(fn);
        if (cur != prevFolder_) { curPos_ = startAt; prevFolder_ = cur; }
    }
    if (sharePairs && group_.continues(fn)) curPos_ -= increment;
    QString r = advancedFormat(mask, [this](auto& t, auto& v) { return getTag(t, v); });
    curPos_ += increment;
    return r;
}
void ActionEnum::save(QXmlStreamWriter& w) const {
    // The Delphi version stored Increment in "Restart" by mistake; fixed here.
    begin(w, "Enum", {{"Mask", mask}, {"Start", startAt}, {"Digits", digits}, {"Incr", increment}, {"Restart", restart}, {"SharePairs", sharePairs}});
    w.writeEndElement();
}
void ActionEnum::load(const XmlElem& e) {
    mask = e.str("Mask", "File %num%%ext%");
    startAt = qBound(-999999999, e.num("Start", 1), 999999999);
    digits = qBound(1, e.num("Digits", 3), 20);
    increment = qBound(-999999, e.num("Incr", 1), 999999);
    restart = e.flag("Restart", false);
    sharePairs = e.flag("SharePairs", false);
}

// --- MP3 ----------------------------------------------------------------------------------
QString ActionMP3::description() const {
    QString r = tr("Generate names with MP3 tags, based on the mask \"%1\"").arg(mask);
    if (twoDigit) r += tr(", insert '0' in track number if required");
    return r;
}
bool ActionMP3::getTag(const QString& tag, QString& v) {
    bool ok = true;
    if (tag == u"author") v = id3_.author.trimmed();
    else if (tag == u"title") v = id3_.title.trimmed();
    else if (tag == u"album") v = id3_.album.trimmed();
    else if (tag == u"year") v = id3_.year.trimmed();
    else if (tag == u"genre") v = id3_.genreText;
    else if (tag == u"comm") v = id3_.comment.trimmed();
    else if (tag == u"track") v = id3_.track ? padNum(id3_.track, twoDigit ? 2 : 1) : QString();
    else ok = Action::getTag(tag, v);
    v = validateFileName(v);
    return ok;
}
QString ActionMP3::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString err;
    if (!readId3(sourcePath(), id3_, &err)) {
        setInfo(err);
        return {};
    }
    return advancedFormat(mask, [this](auto& t, auto& v) { return getTag(t, v); }, AllowIf | AllowQuotes);
}
void ActionMP3::save(QXmlStreamWriter& w) const {
    begin(w, "MP3", {{"Mask", mask}, {"TwoDigit", twoDigit}});
    w.writeEndElement();
}
void ActionMP3::load(const XmlElem& e) {
    mask = e.str("Mask", "%author% - %title%%ext%");
    twoDigit = e.flag("TwoDigit", false);
}

// --- DateTime -----------------------------------------------------------------------------
QString ActionDateTime::description() const {
    QString r = tr("Generate names from files' date and time, based on the mask \"%1\"").arg(mask);
    if (whichDate == 0) r += tr(" using creation date");
    else if (whichDate == 1) r += tr(" using last modification date");
    if (addSuffix) r += tr(", add a suffix if required");
    if (offset != 0) r += tr(" with %1 s offset").arg(offset);
    return r;
}
QString ActionDateTime::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    const QFileInfo fi(sourcePath());
    QDateTime date = adjustFileTime(whichDate == 0 ? fi.birthTime() : fi.lastModified());
    if (!date.isValid()) date = adjustFileTime(fi.lastModified());
    if (!fi.exists() || !date.isValid()) {
        setInfo(tr("Could not get file date"));
        return {};
    }
    date = date.addSecs(offset);
    // f / e outside quotes stand for the file name / extension, inserted as quoted literals.
    QString fmt;
    bool inside = false;
    for (const QChar c : mask) {
        if (!inside && c == u'f') {
            fmt += u'"' + changeFileExt(extractFileName(fn), QString()) + u'"';
        } else if (!inside && c == u'e') {
            if (!plainFolder()) fmt += u'"' + extractFileExt(fn) + u'"';
        } else {
            if (c == u'"') inside = !inside;
            fmt += c;
        }
    }
    QString r = formatDateTime(fmt, date);
    if (addSuffix) r = extractFileName(uniqueFileName(extractFilePath(fn) + r));
    return r;
}
void ActionDateTime::save(QXmlStreamWriter& w) const {
    begin(w, "DateTime", {{"Mask", mask}, {"AddSuffix", addSuffix}, {"WhichDate", whichDate}, {"Offset", offset}});
    w.writeEndElement();
}
void ActionDateTime::load(const XmlElem& e) {
    mask = e.str("Mask", "yyyy'-'mm'-'dde");
    addSuffix = e.flag("AddSuffix", false);
    whichDate = qBound(0, e.num("WhichDate", 1), 1);
    offset = qBound(-999999999, e.num("Offset", 0), 999999999);
}

// --- Random -------------------------------------------------------------------------------
QString ActionRandom::description() const {
    QString r = tr("Generate random names using mask \"%1\"").arg(mask);
    switch (method) {
    case 0: r += tr(", with numbers"); break;
    case 1: r += tr(", using Windows' tick counter"); break;
    case 2: r += tr(", using GUIDs"); break;
    }
    return r;
}
bool ActionRandom::getTag(const QString& tag, QString& v) {
    if (tag != u"random") return Action::getTag(tag, v);
    switch (method) {
    case 0: v = padNum(int(QRandomGenerator::global()->bounded(100000000)), 8); break;
    case 1: v = QString::number(tickCount()); QThread::msleep(10); break;
    case 2: v = QUuid::createUuid().toString().toUpper(); break;
    default: v.clear();
    }
    return true;
}
QString ActionRandom::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    return advancedFormat(mask, [this](auto& t, auto& v) { return getTag(t, v); });
}
void ActionRandom::save(QXmlStreamWriter& w) const {
    begin(w, "Random", {{"Mask", mask}, {"Method", method}});
    w.writeEndElement();
}
void ActionRandom::load(const XmlElem& e) {
    mask = e.str("Mask", "%random%%ext%");
    method = qBound(0, e.num("Method", 0), 2);
}

// --- Case ---------------------------------------------------------------------------------
QString ActionCase::description() const {
    QString r = tr("Change case:");
    switch (caseType) {
    case 0: r += tr(" upper first letter following any of the characters \"%1\"").arg(afterChars); break;
    case 1: r += tr(" upper first letter of the name"); break;
    case 2: r += tr(" all in uppercase"); break;
    case 3: r += tr(" all in lowercase"); break;
    }
    if (useLocale) r += tr(", using Windows' locale settings");
    if (onlyExt) r += tr(", apply to extension instead of name");
    else if (includeExt) r += tr(", also apply to extension");
    return r;
}
QString ActionCase::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString r;
    if (!plainFolder()) {
        r = onlyExt ? extractFileExt(fn)
                    : includeExt ? extractFileName(fn) : changeFileExt(extractFileName(fn), QString());
    } else {
        r = extractFileName(fn);
        if (onlyExt) return r;
    }
    switch (caseType) {
    case 0:
        r = mapCase(r, false, useLocale);
        for (int i = 0; i < r.size(); ++i)
            if (i == 0 || afterChars.contains(r[i - 1])) r[i] = mapCase(r[i], true, useLocale);
        break;
    case 1:
        r = mapCase(r, false, useLocale);
        if (!r.isEmpty()) r[0] = mapCase(r[0], true, useLocale);
        break;
    case 2: r = mapCase(r, true, useLocale); break;
    case 3: r = mapCase(r, false, useLocale); break;
    }
    if (!plainFolder()) {
        if (onlyExt) return changeFileExt(extractFileName(fn), r);
        if (!includeExt) r += extractFileExt(fn);
    }
    return r;
}
void ActionCase::save(QXmlStreamWriter& w) const {
    begin(w, "ChangeCase", {{"Option", caseType}, {"AfterChars", afterChars}, {"UseLocale", useLocale},
                            {"IncludeExt", includeExt}, {"OnlyExt", onlyExt}});
    w.writeEndElement();
}
void ActionCase::load(const XmlElem& e) {
    caseType = qBound(0, e.num("Option", 0), 3);
    afterChars = e.str("AfterChars", "- .+(");
    useLocale = e.flag("UseLocale", true);
    includeExt = e.flag("IncludeExt", false);
    onlyExt = e.flag("OnlyExt", false);
}

// --- FromList -----------------------------------------------------------------------------
QString ActionFromList::description() const {
    QString r = tr("Rename files using specified list");
    if (appendExt) r += tr(", append original extension");
    if (onlyExt) r += tr(", use items as extensions instead of names");
    return r;
}
void ActionFromList::init() { list_ = splitLines(listContents); }
QString ActionFromList::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    QString r = extractFileName(fn);
    if (list_.isEmpty()) {
        setInfo(tr("End of list"));
        return r;
    }
    const QString item = list_.takeFirst();
    if (item.isEmpty()) {
        setInfo(tr("Empty line"));
        return r;
    }
    if (onlyExt) {
        if (!plainFolder()) r = changeFileExt(r, u'.' + item);
    } else {
        r = item;
    }
    if (appendExt && !plainFolder()) r += extractFileExt(fn);
    return r;
}
void ActionFromList::save(QXmlStreamWriter& w) const {
    begin(w, "FromList", {{"AppendExt", appendExt}, {"OnlyExt", onlyExt}});
    cdataChild(w, "listContents", listContents);
    w.writeEndElement();
}
void ActionFromList::load(const XmlElem& e) {
    appendExt = e.flag("AppendExt", false);
    onlyExt = e.flag("OnlyExt", false);
    listContents = e.childText.value("listContents");
}

// --- Regexp -------------------------------------------------------------------------------
QString ActionRegexp::description() const { return tr("Rename files using a regular expression"); }
QString ActionRegexp::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    const QString name = extractFileName(fn);
    if (expr.isEmpty()) return name;  // an empty expression matches everywhere: nothing to do
    const QRegularExpression re = makeRegex(expr);
    if (!re.isValid()) {
        // The preview shows the error as the name; a real run leaves the file alone (setInfo: "not renamed")
        setInfo(tr("Invalid regular expression: %1").arg(re.errorString()));
        return QStringLiteral("<RegExp Error> ") + re.errorString();
    }
    QString out;
    qsizetype last = 0;
    auto it = re.globalMatch(name);
    while (it.hasNext()) {
        const auto m = it.next();
        out += name.mid(last, m.capturedStart() - last) + expandTemplate(repl, m);
        last = m.capturedEnd();
    }
    return out + name.mid(last);
}
void ActionRegexp::save(QXmlStreamWriter& w) const {
    begin(w, "Regexp", {{"Expr", expr}, {"Repl", repl}});
    w.writeEndElement();
}
void ActionRegexp::load(const XmlElem& e) {
    expr = e.str("Expr", QString());
    repl = e.str("Repl", QString());
}

// --- Exif ---------------------------------------------------------------------------------
QString ActionExif::description() const {
    QString r = tr("Generate names with EXIF info, based on the mask \"%1\"").arg(mask);
    if (offset != 0) r += tr(" with %1 s offset").arg(offset);
    return r;
}
bool ActionExif::getTag(const QString& tag, QString& v) {
    bool ok = true;
    if (tag == u"num") {
        v = padNum(curNum_, numDigits);
        return true;
    }
    if (isKnownExifTag(tag)) {
        v.clear();
        for (auto it = tags_.constBegin(); it != tags_.constEnd(); ++it)
            if (it.key().compare(tag, Qt::CaseInsensitive) == 0) v = it.value();
        if (offset != 0 && v.size() == 19) {  // "yyyy:MM:dd HH:mm:ss": shifted as plain calendar fields, whatever the local time zone
            const QDate d = QDate::fromString(v.left(10), QStringLiteral("yyyy:MM:dd"));
            const QTime tm = QTime::fromString(v.mid(11), QStringLiteral("HH:mm:ss"));
            if (d.isValid() && tm.isValid()) {
                const qint64 s = qint64(d.toJulianDay()) * 86400 + QTime(0, 0).secsTo(tm) + offset;
                const qint64 days = s >= 0 ? s / 86400 : -((-s + 86399) / 86400);
                const QDate nd = QDate::fromJulianDay(days);
                const QTime nt = QTime(0, 0).addSecs(int(s - days * 86400));
                if (nd.isValid()) v = nd.toString(QStringLiteral("yyyy:MM:dd")) + u' ' + nt.toString(QStringLiteral("HH:mm:ss"));
            }
        }
    } else {
        ok = Action::getTag(tag, v);
    }
    v = validateFileName(v);
    return ok;
}
QString ActionExif::makeNewName(const QString& fn, bool isFolder) {
    Action::makeNewName(fn, isFolder);
    tags_.clear();
    if (!readExif(sourcePath(), tags_)) {
        setInfo(tr("No EXIF info found"));
        return {};
    }
    if (numRestart) {
        const QString folder = extractFilePath(fn);
        if (folder != prevFolder_) { curNum_ = numStart; prevFolder_ = folder; }
    }
    if (numSharePairs && group_.continues(fn)) curNum_ -= numIncr;
    const QString r = advancedFormat(mask, [this](auto& t, auto& v) { return getTag(t, v); }, AllowIf | AllowQuotes);
    curNum_ += numIncr;
    return r;
}
void ActionExif::save(QXmlStreamWriter& w) const {
    begin(w, "Exif", {{"Mask", mask}, {"Offset", offset}, {"Start", numStart}, {"Digits", numDigits},
                      {"Incr", numIncr}, {"Restart", numRestart}, {"SharePairs", numSharePairs}});
    w.writeEndElement();
}
void ActionExif::load(const XmlElem& e) {
    mask = e.str("Mask", QString());
    offset = qBound(-999999999, e.num("Offset", 0), 999999999);
    numStart = qBound(-999999999, e.num("Start", 1), 999999999);
    numDigits = qBound(1, e.num("Digits", 3), 20);
    numIncr = qBound(-999999, e.num("Incr", 1), 999999);
    numRestart = e.flag("Restart", false);
    numSharePairs = e.flag("SharePairs", false);
}

// --- ActionList ---------------------------------------------------------------------------
namespace {
struct Named { const char* tag; ActionType type; };
const Named kNames[] = {
    {"ChangeExt", ActionType::ChangeExt}, {"StrRepl", ActionType::StringRepl},
    {"MultstrRepl", ActionType::MultstrRepl}, {"StrIns", ActionType::StringInsert},
    {"MoveStr", ActionType::MoveString}, {"CharDel", ActionType::CharDel}, {"Enum", ActionType::Enum},
    {"MP3", ActionType::MP3Tag}, {"DateTime", ActionType::DateTime}, {"Random", ActionType::Random},
    {"ChangeCase", ActionType::Case}, {"FromList", ActionType::FromList},
    {"Regexp", ActionType::Regexp}, {"Exif", ActionType::Exif},
};
}  // namespace

std::unique_ptr<Action> ActionList::create(ActionType t) {
    switch (t) {
    case ActionType::ChangeExt: return std::make_unique<ActionChangeExt>();
    case ActionType::StringRepl: return std::make_unique<ActionStringRepl>();
    case ActionType::MultstrRepl: return std::make_unique<ActionMultstrRepl>();
    case ActionType::StringInsert: return std::make_unique<ActionStringIns>();
    case ActionType::MoveString: return std::make_unique<ActionMoveStr>();
    case ActionType::CharDel: return std::make_unique<ActionCharDel>();
    case ActionType::Enum: return std::make_unique<ActionEnum>();
    case ActionType::MP3Tag: return std::make_unique<ActionMP3>();
    case ActionType::DateTime: return std::make_unique<ActionDateTime>();
    case ActionType::Random: return std::make_unique<ActionRandom>();
    case ActionType::Case: return std::make_unique<ActionCase>();
    case ActionType::FromList: return std::make_unique<ActionFromList>();
    case ActionType::Regexp: return std::make_unique<ActionRegexp>();
    case ActionType::Exif: return std::make_unique<ActionExif>();
    default: return std::make_unique<Action>();
    }
}

Action& ActionList::add(ActionType t) { return add(create(t)); }

Action& ActionList::add(std::unique_ptr<Action> a) {
    items_.push_back(std::move(a));
    items_.back()->position = int(items_.size()) - 1;
    return *items_.back();
}

void ActionList::sort() {
    std::stable_sort(items_.begin(), items_.end(), [](auto& a, auto& b) { return a->position < b->position; });
}

void ActionList::perform(FileList& files, Op op, const Progress& progress) {
    for (size_t i = 0; i < files.size(); ++i) files.at(i).initBeforeJob();
    if (op == Op::Revert) {
        Action().perform(files, Op::Revert, progress);
        return;
    }
    for (auto& a : items_) {
        if (stopFlag()) break;
        a->perform(files, op, progress);
    }
}

bool ActionList::save(const QString& path) const {
    QSaveFile f(path);  // written aside and moved over the old file at the end: a failure leaves the old batch intact
    if (!f.open(QIODevice::WriteOnly)) return false;
    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.setAutoFormattingIndent(1);
    w.writeStartDocument();
    w.writeStartElement(u"AntRenamer"_s);
    w.writeAttribute(u"Version"_s, QCoreApplication::applicationVersion());
    w.writeAttribute(u"Date"_s, QDate::currentDate().toString(Qt::ISODate));
    w.writeStartElement(u"Batch"_s);
    for (auto& a : items_) a->save(w);
    w.writeEndElement();
    w.writeEndElement();
    w.writeEndDocument();
    if (w.hasError()) {
        f.cancelWriting();
        return false;
    }
    return f.commit();
}

bool ActionList::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QXmlStreamReader r(&f);
    std::vector<std::unique_ptr<Action>> loaded;
    bool inBatch = false, sawRoot = false, sawBatch = false;
    while (!r.atEnd()) {
        r.readNext();
        if (r.isEndElement() && inBatch && r.name() == u"Batch") break;
        if (!r.isStartElement()) continue;
        if (!inBatch) {
            if (r.name() == u"AntRenamer") sawRoot = true;
            else if (r.name() == u"Batch" && sawRoot) inBatch = sawBatch = true;
            else break;  // not a batch file
            continue;
        }
        XmlElem e;
        e.name = r.name().toString();
        for (const auto& a : r.attributes()) e.attrs.insert(a.name().toString(), a.value().toString());
        while (!(r.isEndElement() && r.name() == e.name) && !r.atEnd()) {  // direct children: text only
            r.readNext();
            if (r.isStartElement()) {
                const QString child = r.name().toString();
                e.childText.insert(child, r.readElementText());
            }
        }
        for (const Named& n : kNames) {
            if (e.name == QLatin1String(n.tag)) {
                auto a = create(n.type);
                a->load(e);
                a->position = int(loaded.size());
                loaded.push_back(std::move(a));
                break;
            }
        }
    }
    if (r.hasError() || !sawRoot || !sawBatch) return false;  // a foreign XML file must not wipe the current batch
    items_ = std::move(loaded);
    return true;
}

}  // namespace ren
