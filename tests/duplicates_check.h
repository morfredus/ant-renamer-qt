// Same new name for several files (a burst shot in the same second): numbered _01, _02... in the order of the shooting.
// Included by test_core.cpp after CHECK_EQ and touch().
#include <QDateTime>
#include <QRandomGenerator>

namespace dupcheck {

// A file whose content is its own name and whose modification date is `t`.
inline void mk(const QString& dir, const QString& name, const QDateTime& t) {
    QFile f(dir + name);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(name.toUtf8());
        f.flush();  // on Linux the buffered write would otherwise change the date set below
        f.setFileTime(t, QFileDevice::FileModificationTime);
    }
}

// A JPEG with only an EXIF date (DateTimeOriginal) and sub-seconds.
inline void mkExif(const QString& dir, const QString& name, const QString& when, const QString& sub) {
    auto u16 = [](int v) {
        const char b[2] = {char(v & 255), char(v >> 8)};
        return QByteArray(b, 2);
    };
    auto u32 = [&](int v) { return u16(v & 0xffff) + u16(v >> 16); };
    auto entry = [&](int tag, int type, int count, const QByteArray& val) {
        return u16(tag) + u16(type) + u32(count) + (val + QByteArray(4, '\0')).left(4);
    };
    const QByteArray ifd0 = u16(1) + entry(0x8769, 4, 1, u32(26)) + u32(0);
    const QByteArray date = when.toLatin1() + '\0';
    const QByteArray sb = sub.toLatin1() + '\0';
    const QByteArray exif = u16(2) + entry(0x9003, 2, int(date.size()), u32(56)) + entry(0x9291, 2, int(sb.size()), sb) + u32(0);
    const QByteArray tiff = QByteArray("II*\0", 4) + u32(8) + ifd0 + exif + date;
    const QByteArray payload = QByteArray("Exif\0\0", 6) + tiff;
    const int len = int(payload.size()) + 2;
    QFile f(dir + name);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QByteArray("\xff\xd8\xff\xe1", 4) + char(len >> 8) + char(len & 255) + payload + QByteArray("\xff\xda\x00\x02", 4));
        f.write(name.toUtf8());
    }
}

inline QString content(const QString& path) {
    QFile f(path);
    QByteArray b = f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    const int k = b.lastIndexOf(QByteArray("\xff\xda\x00\x02", 4));  // an EXIF test file: the name follows the header
    return QString::fromUtf8(k >= 0 ? b.mid(k + 4) : b);
}

inline QString names(const QString& dir) {
    return QDir(dir).entryList(QDir::Files, QDir::Name).join(u',');
}

inline void rename(const QString& dir, const QStringList& order, ActionList& al) {
    FileList fl;
    for (const QString& n : order) {
        fl.add(dir + n);
    }
    al.perform(fl, Op::Rename);
}

inline ActionList dateAction() {
    ActionList al;
    static_cast<ActionDateTime&>(al.add(ActionType::DateTime)).mask = u"yyyy'-'mm'-'dd'_'hh'-'nn'-'sse"_s;
    return al;
}

}  // namespace dupcheck

static void duplicatesCheck() {
    using namespace dupcheck;
    options().numberDuplicates = true;
    const QDate day(2026, 11, 10);
    auto at = [&](int h, int m, int s, int ms = 0) { return QDateTime(day, QTime(h, m, s, ms)); };

    // a lone photo is left alone, a burst is numbered from _01 in the order of the names, whatever the list order;
    // a name already on the disk makes the new file take a number; the bursts are independent
    {
        QTemporaryDir td;
        const QString d = td.path() + u'/';
        mk(d, u"p1.jpg"_s, at(14, 3, 21));
        for (const QString& b : {u"b1.jpg"_s, u"b2.jpg"_s, u"b3.jpg"_s, u"b4.jpg"_s}) {
            mk(d, b, at(14, 3, 22));
        }
        mk(d, u"p2.jpg"_s, at(14, 3, 23));
        mk(d, u"c1.jpg"_s, at(14, 3, 40));
        mk(d, u"c2.jpg"_s, at(14, 3, 40));
        mk(d, u"2026-11-10_14-03-30.jpg"_s, at(8, 0, 0));  // already there, not in the batch
        mk(d, u"q.jpg"_s, at(14, 3, 30));
        ActionList al = dateAction();
        rename(d, {u"b3.jpg"_s, u"p2.jpg"_s, u"c2.jpg"_s, u"b1.jpg"_s, u"q.jpg"_s, u"b4.jpg"_s, u"p1.jpg"_s, u"c1.jpg"_s,
                   u"b2.jpg"_s},
               al);
        CHECK_EQ(names(d),
                 u"2026-11-10_14-03-21.jpg,2026-11-10_14-03-22_01.jpg,2026-11-10_14-03-22_02.jpg,"
                 u"2026-11-10_14-03-22_03.jpg,2026-11-10_14-03-22_04.jpg,2026-11-10_14-03-23.jpg,"
                 u"2026-11-10_14-03-30.jpg,2026-11-10_14-03-30_01.jpg,2026-11-10_14-03-40_01.jpg,"
                 u"2026-11-10_14-03-40_02.jpg"_s);
        CHECK_EQ(content(d + u"2026-11-10_14-03-22_01.jpg"_s) + content(d + u"2026-11-10_14-03-22_04.jpg"_s), u"b1.jpgb4.jpg"_s);
        CHECK_EQ(content(d + u"2026-11-10_14-03-30_01.jpg"_s), u"q.jpg"_s);
        // run again on the result: nothing moves
        const QString before = names(d);
        FileList fl;
        for (const QString& n : QDir(d).entryList(QDir::Files, QDir::Name)) {
            if (n != u"2026-11-10_14-03-30.jpg"_s) {  // the file that was not in the batch stays out of it
                fl.add(d + n);
            }
        }
        al.perform(fl, Op::Rename);
        CHECK_EQ(names(d), before);
    }

    // the option off: the old behaviour, the later files of a burst are refused
    {
        options().numberDuplicates = false;
        QTemporaryDir td;
        const QString d = td.path() + u'/';
        mk(d, u"b1.jpg"_s, at(14, 3, 22));
        mk(d, u"b2.jpg"_s, at(14, 3, 22));
        ActionList al = dateAction();
        rename(d, {u"b1.jpg"_s, u"b2.jpg"_s}, al);
        CHECK_EQ(names(d), u"2026-11-10_14-03-22.jpg,b2.jpg"_s);
        options().numberDuplicates = true;
    }

    // the file dates tell the order (milliseconds), not the names
    {
        QTemporaryDir td;
        const QString d = td.path() + u'/';
        mk(d, u"a.jpg"_s, at(14, 3, 22, 900));
        mk(d, u"z.jpg"_s, at(14, 3, 22, 100));
        ActionList al = dateAction();
        rename(d, {u"a.jpg"_s, u"z.jpg"_s}, al);
        CHECK_EQ(content(d + u"2026-11-10_14-03-22_01.jpg"_s) + content(d + u"2026-11-10_14-03-22_02.jpg"_s), u"z.jpga.jpg"_s);
    }

    // EXIF: the sub-seconds tell the order (the names say the opposite), and an unreadable one falls back on the name
    {
        QTemporaryDir td;
        const QString d = td.path() + u'/';
        mkExif(d, u"a.jpg"_s, u"2026:11:10 14:03:22"_s, u"80"_s);
        mkExif(d, u"m.jpg"_s, u"2026:11:10 14:03:22"_s, u"20"_s);
        mkExif(d, u"z.jpg"_s, u"2026:11:10 14:03:22"_s, u"5"_s);  // 0.5 s
        ActionList al;
        static_cast<ActionExif&>(al.add(ActionType::Exif)).mask = u"%DateTimeOriginal%%ext%"_s;
        rename(d, {u"z.jpg"_s, u"a.jpg"_s, u"m.jpg"_s}, al);
        const QStringList n = QDir(d).entryList(QDir::Files, QDir::Name);
        CHECK_EQ(QString::number(n.size()), u"3"_s);
        if (n.size() == 3) {
            CHECK_EQ(n[0].contains(u"_01"_s) && n[1].contains(u"_02"_s) && n[2].contains(u"_03"_s) ? u"ok"_s : u"bad"_s, u"ok"_s);
            CHECK_EQ(content(d + n[0]) + content(d + n[1]) + content(d + n[2]), u"m.jpgz.jpga.jpg"_s);
        }
    }

    // a picture and its RAW: same numbers; more than 99 files: wider numbers; a name taken by the batch is skipped
    {
        QTemporaryDir td;
        const QString d = td.path() + u'/';
        std::vector<DupItem> v;
        const QDateTime t = at(14, 3, 22);
        for (const QString& e : {u".jpg"_s, u".dng"_s}) {
            for (const QString& b : {u"x2"_s, u"x1"_s}) {
                mk(d, b + e, t);
                v.push_back({d, u"s"_s + e, d + b + e});
            }
        }
        numberDuplicates(v);
        CHECK_EQ(v[0].name + v[1].name + v[2].name + v[3].name, u"s_02.jpgs_01.jpgs_02.dngs_01.dng"_s);
        std::vector<DupItem> w;
        for (int i = 0; i < 101; ++i) {
            const QString n = u"r%1.jpg"_s.arg(i, 3, 10, QChar(u'0'));
            mk(d, n, t);
            w.push_back({d, u"burst.jpg"_s, d + n});
        }
        std::reverse(w.begin(), w.end());
        numberDuplicates(w);
        CHECK_EQ(w[100].name + u" "_s + w[0].name, u"burst_001.jpg burst_101.jpg"_s);
        // the batch already holds burst_01.jpg: nobody else gets that name
        mk(d, u"burst_01.jpg"_s, t);
        mk(d, u"n1.jpg"_s, t);
        mk(d, u"n2.jpg"_s, t);
        std::vector<DupItem> k = {{d, u"burst.jpg"_s, d + u"n1.jpg"_s},
                                  {d, u"burst.jpg"_s, d + u"n2.jpg"_s},
                                  {d, u"x.jpg"_s, d + u"burst_01.jpg"_s}};
        numberDuplicates(k);
        CHECK_EQ(k[0].name + k[1].name + k[2].name, u"burst_02.jpgburst_03.jpgx.jpg"_s);
    }
}
