// Smallest useful check of the core: previews, real renames on disk, revert, batch XML round-trip.
#include "ren_actions.h"
#include "ren_exif.h"
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
#include "update_check.h"

static void touch(const QString& p) { QFile f(p); if (f.open(QIODevice::WriteOnly)) f.write("x"); }

// Preview a single name through one action (the file does not need to exist).
template <class A, class Fn>
static QString prev(const QString& name, Fn setup, bool isFolder = false) {
    A a;
    setup(a);
#ifdef Q_OS_WIN
    return a.makeNewName(u"C:\\data\\sub\\"_s + name, isFolder);
#else
    return a.makeNewName(u"/data/sub/"_s + name, isFolder);
#endif
}

#include "robust_check.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // paths
#ifdef Q_OS_WIN
    CHECK_EQ(extractFileExt(u"C:\\a.b\\file"_s), u""_s);
#else
    CHECK_EQ(extractFileExt(u"/a.b/file"_s), u""_s);
    CHECK_EQ(extractFileName(u"/data/a:b\\c.txt"_s), u"a:b\\c.txt"_s);  // ':' and '\' are ordinary characters here
    CHECK_EQ(extractFilePath(u"/data/a:b.txt"_s), u"/data/"_s);
    CHECK_EQ(prev<ActionStringIns>(u"a:b.txt"_s, [](auto& a) { a.newString = u"%folder-1%_"_s; a.pos = 0; }), u"data_a:b.txt"_s);
#endif
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
    // 12-hour clock: am/pm, AM/PM, a/p and ampm switch every hour
    CHECK_EQ(formatDateTime(u"hh'-'nn am/pm"_s, QDateTime(QDate(2024, 3, 9), QTime(22, 35, 4))), u"10-35 pm"_s);
    CHECK_EQ(formatDateTime(u"h AM/PM"_s, QDateTime(QDate(2024, 3, 9), QTime(0, 5, 4))), u"12 AM"_s);
    CHECK_EQ(formatDateTime(u"hh a/p"_s, QDateTime(QDate(2024, 3, 9), QTime(12, 5, 4))), u"12 p"_s);
    CHECK_EQ(formatDateTime(u"hh ampm"_s, QDateTime(QDate(2024, 3, 9), QTime(13, 5, 4))), u"01 PM"_s);
    CHECK_EQ(formatDateTime(u"hh \"am/pm\""_s, QDateTime(QDate(2024, 3, 9), QTime(13, 5, 4))), u"13 am/pm"_s);  // quoted: plain text, 24-hour

    // actions (previews)
    CHECK_EQ(prev<ActionChangeExt>(u"a.txt"_s, [](auto& a) { a.newExtension = u".md"_s; }), u"a.md"_s);
    CHECK_EQ(prev<ActionStringRepl>(u"foo bar foo.txt"_s, [](auto& a) { a.search = u"foo"_s; a.replace = u"X"_s; }),
             u"X bar X.txt"_s);
    CHECK_EQ(prev<ActionStringRepl>(u"foo bar foo.txt"_s, [](auto& a) { a.search = u"foo"_s; a.replace = u"X"_s; a.all = false; }),
             u"X bar foo.txt"_s);
    CHECK_EQ(prev<ActionStringRepl>(u"IMG_001_copie (1).jpg"_s, [](auto& a) { a.search = u"IMG_*_copie"_s; a.replace = u"Photo"_s; a.wildcards = true; }),
             u"Photo (1).jpg"_s);
    CHECK_EQ(prev<ActionStringRepl>(u"a1b2.txt"_s, [](auto& a) { a.search = u"a?b"_s; a.replace = u"."_s; a.wildcards = true; }),
             u".2.txt"_s);
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
        QTemporaryDir xd;  // not a fixed name in the shared temp folder: ctest may run the tests in parallel
        const QString xp = xd.filePath(u"ren_exif_test.arb"_s);
        l.save(xp);
        ActionList l2;
        l2.load(xp);
        auto& y = static_cast<ActionExif&>(l2.at(0));
        CHECK_EQ(QString::number(y.numStart) + u"/"_s + QString::number(y.numDigits) + u"/"_s + QString::number(y.numIncr) + (y.numRestart ? u"/r"_s : u""_s), u"5/2/3/r"_s);
    }

    // IPTC + XMP: a synthetic JPEG with an APP13 (UTF-8 IPTC) and an APP1 XMP packet
    {
        auto dataset = [](int set, const QByteArray& v) {
            QByteArray r("\x1c\x02", 2);
            r += char(set); r += char(v.size() >> 8); r += char(v.size() & 255); r += v;
            return r;
        };
        QByteArray iim("\x1c\x01\x5a\x00\x03\x1b%G", 8);
        iim += dataset(25, "red") + dataset(25, "caf\xc3\xa9") + dataset(90, "Paris") + dataset(55, "20240517");
        if (iim.size() % 2) iim += '\0';
        QByteArray ps("Photoshop 3.0\0", 14);
        ps += QByteArray("8BIM\x04\x04\x00\x00", 8);
        const int n = iim.size();
        ps += char(n >> 24); ps += char(n >> 16); ps += char(n >> 8); ps += char(n); ps += iim;
        const QByteArray xmp(
            "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\"><rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">"
            "<rdf:Description xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\" xmp:Rating=\"4\">"
            "<dc:title><rdf:Alt><rdf:li xml:lang=\"x-default\">Sunset</rdf:li></rdf:Alt></dc:title>"
            "<dc:subject><rdf:Bag><rdf:li>sea</rdf:li><rdf:li>sky</rdf:li></rdf:Bag></dc:subject>"
            "<xmp:CreateDate>2024-05-17T14:30:05+02:00</xmp:CreateDate>"
            "</rdf:Description></rdf:RDF></x:xmpmeta>");
        const QByteArray xp = QByteArray("http://ns.adobe.com/xap/1.0/\0", 29) + xmp;
        auto seg = [](char marker, const QByteArray& payload) {
            const int len = payload.size() + 2;
            return QByteArray("\xff", 1) + marker + char(len >> 8) + char(len & 255) + payload;
        };
        QTemporaryDir jd;
        QFile jf(jd.filePath(u"t.jpg"_s));
        if (jf.open(QIODevice::WriteOnly)) jf.write(QByteArray("\xff\xd8", 2) + seg('\xed', ps) + seg('\xe1', xp) + QByteArray("\xff\xda\x00\x02", 4));
        jf.close();
        QMap<QString, QString> t;
        CHECK_EQ(readExif(jf.fileName(), t) ? u"ok"_s : u"none"_s, u"ok"_s);
        CHECK_EQ(t.value(u"IptcKeywords"_s), u"red, caf\u00e9"_s);
        CHECK_EQ(t.value(u"IptcCity"_s), u"Paris"_s);
        CHECK_EQ(t.value(u"IptcDateCreated"_s), u"2024:05:17"_s);
        CHECK_EQ(t.value(u"XmpTitle"_s), u"Sunset"_s);
        CHECK_EQ(t.value(u"XmpKeywords"_s), u"sea, sky"_s);
        CHECK_EQ(t.value(u"XmpRating"_s), u"4"_s);
        CHECK_EQ(t.value(u"XmpCreateDate"_s), u"2024:05:17 14:30:05"_s);
        CHECK_EQ(isKnownExifTag(u"xmpkeywords"_s) && isKnownExifTag(u"IptcCity"_s) ? u"ok"_s : u"no"_s, u"ok"_s);
    }

    // Sidecars: they follow their picture (IMG.xmp, IMG.CR2.xmp, IMG.dop), come back on revert, and are read first
    {
        QTemporaryDir sd;
        const QString d = sd.path() + u'/';
        const QByteArray xmp("<x:xmpmeta xmlns:x=\"adobe:ns:meta/\"><rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">"
                             "<rdf:Description xmlns:xmp=\"http://ns.adobe.com/xap/1.0/\" xmp:Rating=\"3\"/></rdf:RDF></x:xmpmeta>");
        for (const char* n : {"a.jpg", "b.cr2"}) { QFile f(d + QString::fromLatin1(n)); if (f.open(QIODevice::WriteOnly)) f.write("xxxxxxxxxxxxxxxxxxxx"); }
        for (const char* n : {"a.jpg.xmp", "b.xmp", "b.dop"}) { QFile f(d + QString::fromLatin1(n)); if (f.open(QIODevice::WriteOnly)) f.write(xmp); }
        auto has = [&](const char* n) { return QFile::exists(d + QString::fromLatin1(n)) ? u"y"_s : u"n"_s; };
        QMap<QString, QString> t;
        readExif(d + u"b.cr2"_s, t);  // the RAW itself has no metadata: the rating comes from b.xmp
        CHECK_EQ(t.value(u"XmpRating"_s), u"3"_s);

        FileList fl;
        fl.add(d + u"a.jpg"_s);
        fl.add(d + u"b.cr2"_s);
        fl.add(d + u"b.xmp"_s);  // also in the list: it must follow
        CHECK_EQ(fl.at(0).rename(u"z.jpg"_s) ? u"ok"_s : u"fail"_s, u"ok"_s);
        CHECK_EQ(fl.at(1).rename(u"y.cr2"_s) ? u"ok"_s : u"fail"_s, u"ok"_s);
        CHECK_EQ(has("z.jpg.xmp") + has("a.jpg.xmp") + has("y.xmp") + has("b.xmp") + has("y.dop") + has("b.dop"), u"ynynyn"_s);
        CHECK_EQ(fl.at(2).fileName(), u"y.xmp"_s);
        fl.at(1).revert();
        fl.at(0).revert();
        CHECK_EQ(has("a.jpg.xmp") + has("z.jpg.xmp") + has("b.xmp") + has("y.xmp") + has("b.dop") + has("y.dop"), u"ynynyn"_s);

        {   // Windows paths (backslashes), the .xmp being in the list as IMG.CR2.xmp: it must follow too
            QTemporaryDir nd;
            const QString n = QDir::toNativeSeparators(nd.path()) + QDir::separator();
            for (const char* f : {"p.dng", "p.dng.xmp"}) { QFile w(n + QString::fromLatin1(f)); if (w.open(QIODevice::WriteOnly)) w.write(xmp); }
            FileList nl;
            nl.add(n + u"p.dng"_s);
            nl.add(n + u"p.dng.xmp"_s);
            nl.at(0).rename(u"q.dng"_s);
            CHECK_EQ(nl.at(1).fileName(), u"q.dng.xmp"_s);
        }

        options().renameSidecar = false;
        fl.at(0).rename(u"z.jpg"_s);
        CHECK_EQ(has("a.jpg.xmp") + has("z.jpg.xmp"), u"yn"_s);
        options().renameSidecar = true;
    }

    id3Checks();
    updateChecks();
    robustnessChecks();


    // real files: enum + case-only rename + revert + batch round-trip
    QTemporaryDir tmp;
    const QString dir = tmp.path() + u'/';
    {   // date action: f and e stand for the name and the extension outside double quotes, whatever the quotes around
        touch(dir + u"stamp.txt"_s);
        ActionDateTime d;
        d.mask = u"\"x\"f\"y\"e"_s;
        CHECK_EQ(d.makeNewName(dir + u"stamp.txt"_s, false), u"xstampy.txt"_s);
        d.mask = u"yyyy'-'e"_s;  // e is replaced even between single quotes (documented in the help)
        CHECK_EQ(d.makeNewName(dir + u"stamp.txt"_s, false).mid(4), u"-.txt"_s);
        QFile::remove(dir + u"stamp.txt"_s);
    }
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

    {  // JPG + RAW pairs share one number; a repeated extension starts a new one
        QTemporaryDir t2;
        const QString d2 = t2.path() + u'/';
        FileList f2;
        int pos = 0;
        for (const char* n : {"IMG_1.CR2", "IMG_1.JPG", "IMG_1.jpg", "IMG_2.CR2", "IMG_2.JPG"}) {
            touch(d2 + QString::fromLatin1(n));
            f2.add(d2 + QString::fromLatin1(n)).position = pos++;
        }
        ActionList pairs;
        auto& pe = static_cast<ActionEnum&>(pairs.add(ActionType::Enum));
        pe.mask = u"p%num%%ext%"_s;
        pe.digits = 1;
        pe.sharePairs = true;
        pairs.perform(f2, Op::Preview);
        CHECK_EQ(f2.at(0).preview, u"p1.CR2"_s);
        CHECK_EQ(f2.at(1).preview, u"p1.JPG"_s);
        CHECK_EQ(f2.at(2).preview, u"p2.jpg"_s);  // same extension (case-insensitive) as IMG_1.JPG
        CHECK_EQ(f2.at(3).preview, u"p3.CR2"_s);
        CHECK_EQ(f2.at(4).preview, u"p3.JPG"_s);
    }

    batch.perform(files, Op::Revert);
    CHECK_EQ(files.at(1).fileName(), u"b.txt"_s);
    CHECK_EQ(QFile::exists(dir + u"a.txt"_s) ? u"y"_s : u"n"_s, u"y"_s);

    {   // session journal: passes are undone one after the other, newest first
        QTemporaryDir t3;
        const QString d3 = t3.path() + u'/';
        for (const char* n : {"x.txt", "y.txt"}) touch(d3 + QString::fromLatin1(n));
        touch(d3 + u"x.txt.xmp"_s);
        FileList jf;
        Journal journal;
        jf.setJournal(&journal);
        jf.add(d3 + u"x.txt"_s).position = 0;
        jf.add(d3 + u"y.txt"_s).position = 1;

        ActionList p1;  // pass 1: pic1, pic2
        auto& e1 = static_cast<ActionEnum&>(p1.add(ActionType::Enum));
        e1.mask = u"pic%num%%ext%"_s;
        e1.digits = 1;
        p1.perform(jf, Op::Rename);
        ActionList p2;  // pass 2: upper case
        auto& c2 = static_cast<ActionCase&>(p2.add(ActionType::Case));
        c2.caseType = 2;
        c2.includeExt = true;
        p2.perform(jf, Op::Rename);
        ActionList p3;  // pass 3: two actions in one pass
        static_cast<ActionStringRepl&>(p3.add(ActionType::StringRepl)).search = u"PIC"_s;
        auto& e3 = static_cast<ActionEnum&>(p3.add(ActionType::Enum));
        e3.mask = u"z%num%%ext%"_s;
        e3.digits = 1;
        p3.perform(jf, Op::Rename);

        CHECK_EQ(QString::number(journal.size()), u"4"_s);  // the job of pass 3 has two actions: two passes
        CHECK_EQ(QFile::exists(d3 + u"z1.TXT"_s) ? u"y"_s : u"n"_s, u"y"_s);
        CHECK_EQ(QFile::exists(d3 + u"z1.TXT.xmp"_s) ? u"y"_s : u"n"_s, u"y"_s);  // sidecar followed

        CHECK_EQ(QString::number(jf.undoPasses(1)), u"1"_s);  // the last action of the job only
        CHECK_EQ(jf.at(0).fileName(), u"new1.TXT"_s);
        CHECK_EQ(QString::number(jf.undoPasses(1)), u"1"_s);  // then its first action
        CHECK_EQ(jf.at(0).fileName(), u"PIC1.TXT"_s);
        CHECK_EQ(QString::number(journal.size()), u"2"_s);
        CHECK_EQ(QString::number(jf.undoPasses(5)), u"2"_s);  // back to the start, never past it
        CHECK_EQ(jf.at(0).fileName(), u"x.txt"_s);
        CHECK_EQ(jf.at(1).fileName(), u"y.txt"_s);
        CHECK_EQ(QFile::exists(d3 + u"x.txt.xmp"_s) ? u"y"_s : u"n"_s, u"y"_s);
        CHECK_EQ(QString::number(journal.size()), u"0"_s);

        // a file moved outside the program stops the undo, the pass stays
        p1.perform(jf, Op::Rename);
        QFile::rename(d3 + u"pic1.txt"_s, d3 + u"elsewhere.txt"_s);
        CHECK_EQ(QString::number(jf.undoPasses(1)), u"0"_s);
        CHECK_EQ(QString::number(journal.size()), u"1"_s);
    }

    ActionList upper;  // a.txt -> A.TXT must work although the volume is case-insensitive
    static_cast<ActionCase&>(upper.add(ActionType::Case)).caseType = 2;
    static_cast<ActionCase&>(upper.at(0)).includeExt = true;
    upper.perform(files, Op::Rename);
    CHECK_EQ(files.at(0).fileName(), u"A.TXT"_s);
    CHECK_EQ(files.at(0).status() == State::Ok ? u"ok"_s : files.at(0).description(), u"ok"_s);

    {   // a file flagged hidden is left alone while skipHidden is on, and renamed when it is off
        const QString hp = dir + u"h.txt"_s;
        touch(hp);
        FileList hf;
        hf.add(hp).hidden = true;
        ActionList lo;
        static_cast<ActionCase&>(lo.add(ActionType::Case)).caseType = 2;
        lo.perform(hf, Op::Rename);
        CHECK_EQ(hf.at(0).fileName(), u"h.txt"_s);
        options().skipHidden = false;
        lo.perform(hf, Op::Rename);
        options().skipHidden = true;
        CHECK_EQ(hf.at(0).fileName() != u"h.txt"_s ? u"renamed"_s : u"kept"_s, u"renamed"_s);
    }

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
