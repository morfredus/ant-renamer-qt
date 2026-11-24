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
}
