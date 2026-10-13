// Smallest useful check of the core: previews, real renames on disk, revert, batch XML round-trip.
#include "ren_actions.h"
#include "ren_format.h"
#include "ren_id3.h"
#include "ren_path.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

using namespace ren;
using namespace Qt::StringLiterals;

static int failures = 0;
#define CHECK_EQ(actual, expected)                                                                   \
    do {                                                                                             \
        const QString a_ = (actual), e_ = (expected);                                                \
        if (a_ != e_) {                                                                              \
            ++failures;                                                                              \
            std::printf("FAIL %s:%d  %s\n   got: %s\n  want: %s\n", __FILE__, __LINE__, #actual,     \
                        qPrintable(a_), qPrintable(e_));                                             \
        }                                                                                            \
    } while (0)

#include "id3_check.h"

static void touch(const QString& p) { QFile f(p); if (f.open(QIODevice::WriteOnly)) f.write("x"); }

// Preview a single name through one action (the file does not need to exist).
template <class A, class Fn>
static QString prev(const QString& name, Fn setup, bool isFolder = false) {
    A a;
    setup(a);
    return a.makeNewName(u"C:\\data\\sub\\"_s + name, isFolder);
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // paths
    CHECK_EQ(extractFileExt(u"C:\\a.b\\file"_s), u""_s);
    CHECK_EQ(extractFileExt(u"x/y.tar.gz"_s), u".gz"_s);
    CHECK_EQ(changeFileExt(u"a.txt"_s, u".md"_s), u"a.md"_s);

    // advancedFormat
    auto get = [](const QString& t, QString& v) {
        if (t == u"a") { v = u"AA"_s; return true; }
        if (t == u"e") { v.clear(); return true; }
        return false;
    };
    CHECK_EQ(advancedFormat(u"%a%-%%-%zz%"_s, get), u"AA-%-%zz%"_s);
    CHECK_EQ(advancedFormat(u"$if(%a%,yes,no)"_s, get, AllowIf | AllowQuotes), u"yes"_s);
    CHECK_EQ(advancedFormat(u"$if(%e%,yes,no)!"_s, get, AllowIf | AllowQuotes), u"no!"_s);
    CHECK_EQ(advancedFormat(u"$if(%a%,\" (\"%a%\")\",)"_s, get, AllowIf | AllowQuotes), u" (AA)"_s);
    CHECK_EQ(advancedFormat(u"cost $5"_s, get, AllowIf), u"cost 5"_s);  // lone $ is dropped (as in Delphi) and must not hang

    // formatDateTime
    CHECK_EQ(formatDateTime(u"yyyy'-'mm'-'dd hh:nn:ss"_s, QDateTime(QDate(2024, 3, 9), QTime(7, 5, 4))),
             u"2024-03-09 07:05:04"_s);
    CHECK_EQ(formatDateTime(u"hh:mm"_s, QDateTime(QDate(2024, 3, 9), QTime(7, 5, 4))), u"07:05"_s);

    // actions (previews)
    CHECK_EQ(prev<ActionChangeExt>(u"a.txt"_s, [](auto& a) { a.newExtension = u".md"_s; }), u"a.md"_s);
    CHECK_EQ(prev<ActionStringRepl>(u"foo bar foo.txt"_s, [](auto& a) { a.search = u"foo"_s; a.replace = u"X"_s; }),
             u"X bar X.txt"_s);
    CHECK_EQ(prev<ActionStringRepl>(u"foo bar foo.txt"_s, [](auto& a) { a.search = u"foo"_s; a.replace = u"X"_s; a.all = false; }),
             u"X bar foo.txt"_s);
    CHECK_EQ(prev<ActionStringIns>(u"abc.txt"_s, [](auto& a) { a.newString = u"__"_s; a.pos = 1; }), u"a__bc.txt"_s);
    CHECK_EQ(prev<ActionStringIns>(u"abc.txt"_s, [](auto& a) { a.newString = u"%folder1%-"_s; a.pos = 0; }), u"sub-abc.txt"_s);
    CHECK_EQ(prev<ActionMoveStr>(u"abcdef.txt"_s, [](auto& a) { a.fromPos = 0; a.count = 2; a.toBegin = false; a.toPos = 0; }),
             u"cdefab.txt"_s);
    CHECK_EQ(prev<ActionCharDel>(u"abcdef.txt"_s, [](auto& a) { a.nbChar = 2; a.pos = 1; }), u"adef.txt"_s);
    CHECK_EQ(prev<ActionCharDel>(u"abcdef.txt"_s, [](auto& a) { a.nbChar = 2; a.afterString = 1; a.str = u"b"_s; }), u"abef.txt"_s);
    CHECK_EQ(prev<ActionCase>(u"hello big world.TXT"_s, [](auto& a) { a.caseType = 0; }), u"Hello Big World.TXT"_s);
    CHECK_EQ(prev<ActionCase>(u"hello.txt"_s, [](auto& a) { a.caseType = 2; a.onlyExt = true; }), u"hello.TXT"_s);
    CHECK_EQ(prev<ActionRegexp>(u"IMG_0042.jpg"_s, [](auto& a) { a.expr = u"^img_(\\d+)"_s; a.repl = u"photo-$1"_s; }),
             u"photo-0042.jpg"_s);
    CHECK_EQ(prev<ActionRegexp>(u"a.txt"_s, [](auto& a) { a.expr = u"("_s; }).startsWith(u"<RegExp Error>"_s) ? u"ok"_s : u"bad"_s, u"ok"_s);
    CHECK_EQ(prev<ActionMultstrRepl>(u"caf\u00e9.txt"_s, [](auto& a) { a.setChars = u"\u00e9\te\n"_s; }), u"cafe.txt"_s);

    // EXIF %num% counter (the file has no EXIF, so only the XML round-trip is checked)
    {
        ActionList l;
        auto& x = static_cast<ActionExif&>(l.add(ActionType::Exif));
        x.mask = u"%num%"_s; x.numStart = 5; x.numDigits = 2; x.numIncr = 3; x.numRestart = true;
        const QString xp = QDir::temp().filePath(u"ren_exif_test.arb"_s);
        l.save(xp);
        ActionList l2;
        l2.load(xp);
        auto& y = static_cast<ActionExif&>(l2.at(0));
        CHECK_EQ(QString::number(y.numStart) + u"/"_s + QString::number(y.numDigits) + u"/"_s + QString::number(y.numIncr) + (y.numRestart ? u"/r"_s : u""_s), u"5/2/3/r"_s);
        QFile::remove(xp);
    }

    id3Checks();


    // real files: enum + case-only rename + revert + batch round-trip
    QTemporaryDir tmp;
    const QString dir = tmp.path() + u'/';
    touch(dir + u"b.txt"_s);
    touch(dir + u"a.txt"_s);

    FileList files;
    files.add(dir + u"a.txt"_s).position = 0;
    files.add(dir + u"b.txt"_s).position = 1;

    ActionList batch;
    auto& en = static_cast<ActionEnum&>(batch.add(ActionType::Enum));
    en.mask = u"pic%num%%ext%"_s;
    en.digits = 2;

    batch.perform(files, Op::Preview);
    CHECK_EQ(files.at(0).preview, u"pic01.txt"_s);
    CHECK_EQ(files.at(1).preview, u"pic02.txt"_s);

    batch.perform(files, Op::Rename);
    CHECK_EQ(files.at(0).fileName(), u"pic01.txt"_s);
    CHECK_EQ(QFile::exists(dir + u"pic02.txt"_s) ? u"y"_s : u"n"_s, u"y"_s);

    batch.perform(files, Op::Revert);
    CHECK_EQ(files.at(1).fileName(), u"b.txt"_s);
    CHECK_EQ(QFile::exists(dir + u"a.txt"_s) ? u"y"_s : u"n"_s, u"y"_s);

    ActionList upper;  // a.txt -> A.TXT must work although the volume is case-insensitive
    static_cast<ActionCase&>(upper.add(ActionType::Case)).caseType = 2;
    static_cast<ActionCase&>(upper.at(0)).includeExt = true;
    upper.perform(files, Op::Rename);
    CHECK_EQ(files.at(0).fileName(), u"A.TXT"_s);
    CHECK_EQ(files.at(0).status() == State::Ok ? u"ok"_s : files.at(0).description(), u"ok"_s);

    const QString xml = dir + u"batch.xml"_s;
    ActionList mixed;
    static_cast<ActionStringRepl&>(mixed.add(ActionType::StringRepl)).search = u"\u00e9 & <x>"_s;
    static_cast<ActionFromList&>(mixed.add(ActionType::FromList)).listContents = u"one\ntwo"_s;
    CHECK_EQ(mixed.save(xml) ? u"ok"_s : u"bad"_s, u"ok"_s);
    ActionList again;
    CHECK_EQ(again.load(xml) ? u"ok"_s : u"bad"_s, u"ok"_s);
    CHECK_EQ(QString::number(again.size()), u"2"_s);
    CHECK_EQ(again.at(0).description(), mixed.at(0).description());
    CHECK_EQ(again.at(1).description(), mixed.at(1).description());
    CHECK_EQ(static_cast<ActionFromList&>(again.at(1)).listContents, u"one\ntwo"_s);

    std::printf(failures ? "%d FAILURE(S)\n" : "all core checks passed\n", failures);
    return failures ? 1 : 0;
}
