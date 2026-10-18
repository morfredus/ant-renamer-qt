// Regression checks for the audit of 3.0.0: hostile or odd input must not crash the readers, rename the wrong thing or
// lose data. Included by test_core.cpp after CHECK_EQ is defined.
#pragma once

#include <QDateTime>
#include <QTemporaryDir>

inline QString okBad(bool ok) { return ok ? QStringLiteral("ok") : QStringLiteral("bad"); }

inline void writeFile(const QString& path, const QByteArray& data) {
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) f.write(data);
}

// Little-endian helpers to build tiny TIFF/EXIF files.
inline QByteArray le16(int v) { return QByteArray().append(char(v)).append(char(v >> 8)); }
inline QByteArray le32(quint32 v) { return le16(int(v & 0xFFFF)) + le16(int(v >> 16)); }
inline QByteArray ifdEntry(int tag, int type, quint32 count, quint32 value) {
    return le16(tag) + le16(type) + le32(count) + le32(value);
}

inline void robustnessChecks() {
    using namespace ren;
    QTemporaryDir td;
    const QString dir = td.path() + u'/';

    // --- ID3v1: a blank year is NUL padded and must read as empty (a NUL in a name cuts it and drops the extension)
    {
        QByteArray v1(128, '\0');
        v1.replace(0, 3, "TAG");
        v1.replace(3, 4, "Song");
        writeFile(dir + u"v1.mp3"_s, QByteArray(200, 'x') + v1);
        Id3Info info;
        CHECK_EQ(okBad(readId3(dir + u"v1.mp3"_s, info)), u"ok"_s);
        CHECK_EQ(info.title + u'|' + info.year + u'|' + QString::number(info.track), u"Song||0"_s);
    }
    // --- ID3v1.0 (comment text up to the last byte) has no track number; v1.1 (byte 125 == 0) has one
    {
        QByteArray v10(128, '\0');
        v10.replace(0, 3, "TAG");
        v10.replace(97, 30, QByteArray(30, 'c'));
        writeFile(dir + u"v10.mp3"_s, QByteArray(200, 'x') + v10);
        Id3Info a;
        readId3(dir + u"v10.mp3"_s, a);
        CHECK_EQ(QString::number(a.track), u"0"_s);
        QByteArray v11(128, '\0');
        v11.replace(0, 3, "TAG");
        v11[126] = char(3);
        writeFile(dir + u"v11.mp3"_s, QByteArray(200, 'x') + v11);
        Id3Info b;
        readId3(dir + u"v11.mp3"_s, b);
        CHECK_EQ(QString::number(b.track), u"3"_s);
    }
    // --- ID3v2.3 with a hostile extended header size: must return, not read before the buffer
    for (const char* sizeField : {"\x7F\xFF\xFF\xFC", "\xFF\xFF\xFF\xF0", "\x00\x00\x00\x00"}) {
        QByteArray body(QByteArray(sizeField, 4) + QByteArray(40, '\0'));
        const int n = int(body.size());
        QByteArray tag = QByteArray("ID3\x03\x00\x40", 6);  // v2.3, flag 0x40: extended header
        tag.append(char((n >> 21) & 0x7F)).append(char((n >> 14) & 0x7F)).append(char((n >> 7) & 0x7F)).append(char(n & 0x7F));
        writeFile(dir + u"hostile.mp3"_s, tag + body + QByteArray(30, 'x'));
        Id3Info info;
        readId3(dir + u"hostile.mp3"_s, info);  // reaching the next line is the check
        CHECK_EQ(u"survived"_s, u"survived"_s);
    }

    // --- EXIF/IPTC: hostile offsets in a bare TIFF must not read outside the buffer
    {
        const QByteArray head("II*\0", 4);
        // IFD offset near INT_MAX
        writeFile(dir + u"t1.tif"_s, head + le32(0x7FFFFFFE) + QByteArray(32, '\0'));
        // one entry (ExposureTime, RATIONAL) whose data offset is near INT_MAX
        writeFile(dir + u"t2.tif"_s, head + le32(8) + le16(1) + ifdEntry(0x829A, 5, 1, 0x7FFFFFF0) + le32(0) + QByteArray(16, '\0'));
        // entry count larger than the data, and an Exif sub-IFD pointer to itself
        writeFile(dir + u"t3.tif"_s, head + le32(8) + le16(500) + ifdEntry(0x8769, 4, 1, 8) + QByteArray(8, '\0'));
        // big-endian header with a truncated IFD
        writeFile(dir + u"t4.tif"_s, QByteArray("MM\0*", 4) + QByteArray("\0\0\0\x08", 4) + QByteArray("\0\x05", 2));
        // a file that starts with "MM" but is not TIFF: ignored
        writeFile(dir + u"t5.txt"_s, QByteArray("MMmm this is a text file, not a picture") + QByteArray(40, ' '));
        for (const char* n : {"t1.tif", "t2.tif", "t3.tif", "t4.tif", "t5.txt"}) {
            QMap<QString, QString> tags;
            readExif(dir + QString::fromLatin1(n), tags);
        }
        CHECK_EQ(u"survived"_s, u"survived"_s);
        QMap<QString, QString> none;
        CHECK_EQ(okBad(!readExif(dir + u"t5.txt"_s, none)), u"ok"_s);
    }

    // --- EXIF date offset: plain calendar arithmetic, the same in every time zone (no daylight saving jump)
    {
        auto exifNameWithOffset = [&](const char* date, int offset) {
            const QByteArray s = QByteArray(date) + '\0';  // 20 bytes with the terminator
            const quint32 exifIfd = 26, dataOff = 44;
            QByteArray tiff = QByteArray("II*\0", 4) + le32(8) + le16(1) + ifdEntry(0x8769, 4, 1, exifIfd) + le32(0);
            tiff += le16(1) + ifdEntry(0x9003, 2, quint32(s.size()), dataOff) + le32(0) + s;
            const QString path = dir + u"dt.tif"_s;
            writeFile(path, tiff);
            ActionExif a;
            a.mask = u"%DateTimeOriginal%"_s;
            a.offset = offset;
            return a.makeNewName(path, false);
        };
        CHECK_EQ(exifNameWithOffset("2024:03:31 01:30:00", 3600), u"2024-03-31 02-30-00"_s);   // Europe/Paris spring forward
        CHECK_EQ(exifNameWithOffset("2024:10:27 02:30:00", 3600), u"2024-10-27 03-30-00"_s);   // Europe/Paris fall back
        CHECK_EQ(exifNameWithOffset("2024:12:31 23:30:00", 3600), u"2025-01-01 00-30-00"_s);
        CHECK_EQ(exifNameWithOffset("2024:01:01 00:10:00", -3600), u"2023-12-31 23-10-00"_s);
    }

    // --- an invalid regular expression: the preview shows the error, a real run leaves the file alone
    {
        const QString f = dir + u"keep.txt"_s;
        writeFile(f, "x");
        FileList files;
        files.add(f).position = 0;
        ActionList batch;
        static_cast<ActionRegexp&>(batch.add(ActionType::Regexp)).expr = u"("_s;
        batch.perform(files, Op::Preview);
        CHECK_EQ(okBad(files.at(0).preview.startsWith(u"<RegExp Error>"_s)), u"ok"_s);
        batch.perform(files, Op::Rename);
        CHECK_EQ(QFile::exists(f) ? u"kept"_s : u"renamed"_s, u"kept"_s);
        CHECK_EQ(files.at(0).status() == State::Ok ? u"ok"_s : u"not renamed"_s, u"not renamed"_s);
    }
    // --- an empty expression changes nothing (it used to put the replacement between every character)
    CHECK_EQ(prev<ActionRegexp>(u"abc.txt"_s, [](auto& a) { a.expr.clear(); a.repl = u"X"_s; }), u"abc.txt"_s);

    // --- a new name that contains a NUL is refused, never truncated by the system
    {
        const QString f = dir + u"nulchar.txt"_s;  // not "nul.txt": a reserved device name on some Windows versions
        writeFile(f, "x");
        File file(f);
        CHECK_EQ(okBad(!file.rename(dir + u"a"_s + QChar(0) + u"b.txt"_s)), u"ok"_s);
        CHECK_EQ(file.status() == State::Error ? u"error"_s : u"other"_s, u"error"_s);
        CHECK_EQ(QFile::exists(f) ? u"kept"_s : u"gone"_s, u"kept"_s);
    }

    // --- "apply to extension" never leaves a dot at the end
    CHECK_EQ(prev<ActionStringRepl>(u"Makefile"_s, [](auto& a) { a.search = u"x"_s; a.replace = u"y"_s; a.onlyExt = true; }), u"Makefile"_s);
    CHECK_EQ(prev<ActionStringRepl>(u"a.txt"_s, [](auto& a) { a.search = u"txt"_s; a.replace.clear(); a.onlyExt = true; }), u"a"_s);
    CHECK_EQ(prev<ActionStringIns>(u"file"_s, [](auto& a) { a.newString.clear(); a.ext = true; }), u"file"_s);
    CHECK_EQ(prev<ActionCharDel>(u"file.t"_s, [](auto& a) { a.nbChar = 1; a.pos = 0; a.ext = true; }), u"file"_s);

    // --- date mask: repeated 'a' keeps its letters
    CHECK_EQ(formatDateTime(u"aab"_s, QDateTime(QDate(2024, 3, 9), QTime(10, 0))), u"aab"_s);

    // --- batch files: a foreign XML is refused and the current batch survives; values are clamped to the editor ranges
    {
        ActionList keep;
        keep.add(ActionType::Case);
        writeFile(dir + u"foreign.xml"_s, "<foo><bar/></foo>");
        CHECK_EQ(okBad(!keep.load(dir + u"foreign.xml"_s)), u"ok"_s);
        CHECK_EQ(QString::number(keep.size()), u"1"_s);
        writeFile(dir + u"huge.arb"_s,
                  "<AntRenamer><Batch><Enum Mask=\"%num%\" Digits=\"2000000000\" Start=\"5\" Incr=\"1\"/>"
                  "<StrIns Str=\"x\" Pos=\"2000000000\"/></Batch></AntRenamer>");
        ActionList huge;
        CHECK_EQ(okBad(huge.load(dir + u"huge.arb"_s)), u"ok"_s);
        CHECK_EQ(QString::number(static_cast<ActionEnum&>(huge.at(0)).digits), u"20"_s);
        CHECK_EQ(QString::number(static_cast<ActionStringIns&>(huge.at(1)).pos), u"1000"_s);
        // saving over an existing batch works and leaves a readable file
        CHECK_EQ(okBad(huge.save(dir + u"huge.arb"_s)), u"ok"_s);
        ActionList back;
        CHECK_EQ(okBad(back.load(dir + u"huge.arb"_s)), u"ok"_s);
        CHECK_EQ(QString::number(back.size()), u"2"_s);
    }
}
