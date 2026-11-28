// Filter by date, time of the day and size (FileFilter). Included by test_core.cpp after duplicates_check.h.
#include <QTemporaryDir>

inline void filterCheck() {
    using dupcheck::mk;
    using dupcheck::mkExif;
    QTemporaryDir t;
    const QString d = t.path() + u'/';
    auto at = [](int y, int mo, int day, int h, int mi) { return QDateTime(QDate(y, mo, day), QTime(h, mi)); };
    mk(d, u"a.txt"_s, at(2024, 5, 1, 14, 30));   // afternoon
    mk(d, u"b.txt"_s, at(2024, 5, 1, 9, 5));     // morning
    mk(d, u"c.txt"_s, at(2024, 6, 15, 23, 30));  // late evening, a month later
    mk(d, u"d.txt"_s, at(2024, 6, 16, 1, 15));   // just after midnight
    auto names = [&](const FileFilter& flt) {
        QStringList out;
        for (const char* n : {"a.txt", "b.txt", "c.txt", "d.txt"}) {
            const File f(d + QString::fromLatin1(n));
            if (flt.matches(f)) {
                out << QString::fromLatin1(n);
            }
        }
        return out.join(u',');
    };

    FileFilter none;
    CHECK_EQ(none.isEmpty() ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(names(none), u"a.txt,b.txt,c.txt,d.txt"_s);  // no criterion: everything stays

    FileFilter period;  // a period, both ends included
    period.from = at(2024, 5, 1, 9, 5);
    period.to = at(2024, 5, 1, 14, 30);
    CHECK_EQ(names(period), u"a.txt,b.txt"_s);
    FileFilter after;  // only a start
    after.from = at(2024, 6, 1, 0, 0);
    CHECK_EQ(names(after), u"c.txt,d.txt"_s);

    FileFilter hours;  // "between 14:00 and 15:00", whatever the day
    hours.timeFrom = QTime(14, 0);
    hours.timeTo = QTime(15, 0);
    CHECK_EQ(names(hours), u"a.txt"_s);
    FileFilter night;  // across midnight
    night.timeFrom = QTime(23, 0);
    night.timeTo = QTime(2, 0);
    CHECK_EQ(names(night), u"c.txt,d.txt"_s);
    FileFilter both;  // a period AND a time of the day
    both.from = at(2024, 6, 1, 0, 0);
    both.timeFrom = QTime(23, 0);
    both.timeTo = QTime(23, 59);
    CHECK_EQ(names(both), u"c.txt"_s);

    // size: a.txt .. d.txt hold their own name (5 bytes each); a bigger file and a folder
    QFile big(d + u"big.bin"_s);
    CHECK_EQ(big.open(QIODevice::WriteOnly) ? u"y"_s : u"n"_s, u"y"_s);
    big.write(QByteArray(2000, 'z'));
    big.close();
    QDir().mkdir(d + u"sub"_s);
    FileFilter large;
    large.minSize = 1000;
    CHECK_EQ(large.matches(File(d + u"big.bin"_s)) ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(large.matches(File(d + u"a.txt"_s)) ? u"y"_s : u"n"_s, u"n"_s);
    CHECK_EQ(large.matches(File(d + u"sub"_s)) ? u"y"_s : u"n"_s, u"y"_s);  // a size limit does not concern folders
    FileFilter small;
    small.maxSize = 10;
    CHECK_EQ(small.matches(File(d + u"a.txt"_s)) ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(small.matches(File(d + u"big.bin"_s)) ? u"y"_s : u"n"_s, u"n"_s);

    // the shooting date comes from the EXIF of the file, not from its modification date
    mkExif(d, u"p.jpg"_s, u"2023:08:12 14:20:00"_s, u"000"_s);
    QFile pj(d + u"p.jpg"_s);
    CHECK_EQ(pj.open(QIODevice::ReadWrite) ? u"y"_s : u"n"_s, u"y"_s);
    pj.setFileTime(at(2025, 1, 1, 3, 0), QFileDevice::FileModificationTime);
    pj.close();
    FileFilter shot;
    shot.kind = FileFilter::DateKind::Shot;
    shot.timeFrom = QTime(14, 0);
    shot.timeTo = QTime(15, 0);
    CHECK_EQ(shot.matches(File(d + u"p.jpg"_s)) ? u"y"_s : u"n"_s, u"y"_s);
    FileFilter modified = shot;  // the same hours on the modification date (03:00): no
    modified.kind = FileFilter::DateKind::Modified;
    CHECK_EQ(modified.matches(File(d + u"p.jpg"_s)) ? u"y"_s : u"n"_s, u"n"_s);
    FileFilter shotNoExif = shot;  // a file without EXIF falls back on its modification date
    shotNoExif.timeFrom = QTime(14, 0);
    shotNoExif.timeTo = QTime(15, 0);
    CHECK_EQ(shotNoExif.matches(File(d + u"a.txt"_s)) ? u"y"_s : u"n"_s, u"y"_s);

    // by name: masks (several, * and ?, whole name, not case sensitive) and regular expressions (anywhere in the name)
    const QStringList sample = {u"IMG_0001.JPG"_s, u"IMG_0002.png"_s, u"notes.txt"_s, u"a.txt"_s, u"photo.jpeg"_s};
    auto hits = [&](const NameFilter& nf) {
        QStringList out;
        for (const QString& n : sample) {
            if (nf.matches(n)) {
                out << n;
            }
        }
        return out.join(u',');
    };
    NameFilter none2;
    CHECK_EQ(none2.isEmpty() ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(hits(none2), sample.join(u','));  // no pattern: everything
    NameFilter jpg;
    jpg.pattern = u"*.jpg"_s;  // the whole name, extension included, capitals ignored
    CHECK_EQ(hits(jpg), u"IMG_0001.JPG"_s);
    jpg.caseSensitive = true;
    CHECK_EQ(hits(jpg), QString());
    NameFilter masks;
    masks.pattern = u"*.png; *.txt ;IMG_000?.JPG"_s;  // several masks, blanks around them ignored
    CHECK_EQ(hits(masks), u"IMG_0001.JPG,IMG_0002.png,notes.txt,a.txt"_s);
    NameFilter dots;
    dots.pattern = u"a.txt"_s;  // the dot of a mask is a dot, not "any character"
    CHECK_EQ(hits(dots), u"a.txt"_s);
    NameFilter rx;
    rx.regex = true;
    rx.pattern = u"^IMG_\\d+\\."_s;
    CHECK_EQ(hits(rx), u"IMG_0001.JPG,IMG_0002.png"_s);
    rx.pattern = u"txt"_s;  // found anywhere
    CHECK_EQ(hits(rx), u"notes.txt,a.txt"_s);
    rx.pattern = u"(unclosed"_s;
    CHECK_EQ(rx.error().isEmpty() ? u"y"_s : u"n"_s, u"n"_s);  // not usable: says why, matches nothing
    CHECK_EQ(hits(rx), QString());
    CHECK_EQ(masks.error().isEmpty() ? u"y"_s : u"n"_s, u"y"_s);
    FileFilter byName;  // inside a filter, with the other criteria
    byName.name.pattern = u"*.txt"_s;
    CHECK_EQ(byName.isEmpty() ? u"y"_s : u"n"_s, u"n"_s);
    CHECK_EQ(names(byName), u"a.txt,b.txt,c.txt,d.txt"_s);
    byName.from = at(2024, 6, 1, 0, 0);
    CHECK_EQ(names(byName), u"c.txt,d.txt"_s);  // the name AND the period
    byName.name.pattern = u"a*"_s;
    CHECK_EQ(names(byName), QString());

    // values of the command line
    qint64 n = 0;
    CHECK_EQ(parseFilterSize(u"500"_s, n) && n == 500 ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterSize(u"10K"_s, n) && n == 10240 ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterSize(u" 2.5 mb "_s, n) && n == 2621440 ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterSize(u"1G"_s, n) && n == (1ll << 30) ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterSize(u"1,5K"_s, n) && n == 1536 ? u"y"_s : u"n"_s, u"y"_s);  // a French decimal comma
    CHECK_EQ(parseFilterSize(u"big"_s, n) ? u"y"_s : u"n"_s, u"n"_s);
    CHECK_EQ(parseFilterSize(u"-5"_s, n) ? u"y"_s : u"n"_s, u"n"_s);
    QTime tm;
    CHECK_EQ(parseFilterTime(u"14:05"_s, tm) && tm == QTime(14, 5) ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterTime(u"23:59:30"_s, tm) && tm == QTime(23, 59, 30) ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterTime(u"25:00"_s, tm) ? u"y"_s : u"n"_s, u"n"_s);
    CHECK_EQ(parseFilterTime(u"noon"_s, tm) ? u"y"_s : u"n"_s, u"n"_s);
    QDateTime dt;
    CHECK_EQ(parseFilterDate(u"2024-05-01"_s, false, dt) && dt == at(2024, 5, 1, 0, 0) ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterDate(u"2024-05-01"_s, true, dt) && dt == QDateTime(QDate(2024, 5, 1), QTime(23, 59, 59, 999)) ? u"y"_s : u"n"_s,
             u"y"_s);  // "to" a date keeps that whole day
    CHECK_EQ(parseFilterDate(u"2024-05-01T14:30"_s, true, dt) && dt == at(2024, 5, 1, 14, 30) ? u"y"_s : u"n"_s, u"y"_s);
    CHECK_EQ(parseFilterDate(u"2024-05-01 14:30:15"_s, false, dt) && dt == QDateTime(QDate(2024, 5, 1), QTime(14, 30, 15)) ? u"y"_s : u"n"_s,
             u"y"_s);
    CHECK_EQ(parseFilterDate(u"01/05/2024"_s, false, dt) ? u"y"_s : u"n"_s, u"n"_s);  // only the ISO order, no guessing
    CHECK_EQ(parseFilterDate(u"2024-13-40"_s, false, dt) ? u"y"_s : u"n"_s, u"n"_s);
}
