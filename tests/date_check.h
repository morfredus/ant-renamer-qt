// "Set the dates from EXIF": the modification date becomes the EXIF shooting date, safeguards, undo. Included by test_core.cpp
// after filter_check.h.
#include <QTemporaryDir>

inline void dateCheck() {
    using dupcheck::mk;
    using dupcheck::mkExif;
    QTemporaryDir t;
    const QString d = t.path() + u'/';
    auto at = [](int y, int mo, int day, int h, int mi, int s = 0) { return QDateTime(QDate(y, mo, day), QTime(h, mi, s)); };
    auto yn = [](bool b) { return b ? u"y"_s : u"n"_s; };
    auto mtime = [](const QString& p) { return QFileInfo(p).lastModified().toSecsSinceEpoch(); };

    mkExif(d, u"p.jpg"_s, u"2023:08:12 14:20:05"_s, u"000"_s);  // shot in 2023, copied in 2025
    QFile pj(d + u"p.jpg"_s);
    CHECK_EQ(yn(pj.open(QIODevice::ReadWrite)), u"y"_s);
    pj.setFileTime(at(2025, 1, 1, 3, 0), QFileDevice::FileModificationTime);
    pj.close();
    mkExif(d, u"same.jpg"_s, u"2022:01:02 03:04:05"_s, u"000"_s);  // already right
    QFile sj(d + u"same.jpg"_s);
    CHECK_EQ(yn(sj.open(QIODevice::ReadWrite)), u"y"_s);
    sj.setFileTime(at(2022, 1, 2, 3, 4, 5), QFileDevice::FileModificationTime);
    sj.close();
    mk(d, u"plain.txt"_s, at(2024, 4, 4, 4, 4));  // no EXIF: must stay as it is
    mkExif(d, u"zero.jpg"_s, u"0000:00:00 00:00:00"_s, u"000"_s);  // a camera without a clock: no invented date
    QFile zj(d + u"zero.jpg"_s);
    CHECK_EQ(yn(zj.open(QIODevice::ReadWrite)), u"y"_s);
    zj.setFileTime(at(2024, 9, 9, 9, 9), QFileDevice::FileModificationTime);
    zj.close();
    mkExif(d, u"hid.jpg"_s, u"2021:05:05 05:05:05"_s, u"000"_s);
    QFile hj(d + u"hid.jpg"_s);
    CHECK_EQ(yn(hj.open(QIODevice::ReadWrite)), u"y"_s);
    hj.setFileTime(at(2024, 8, 8, 8, 8), QFileDevice::FileModificationTime);
    hj.close();
    QDir().mkdir(d + u"sub"_s);

    // the plan says what would happen, and touches nothing
    CHECK_EQ(yn(planDate(File(d + u"p.jpg"_s)).kind == DatePlan::Kind::Change), u"y"_s);
    CHECK_EQ(planDate(File(d + u"p.jpg"_s)).shot.toString(Qt::ISODate), u"2023-08-12T14:20:05"_s);
    CHECK_EQ(yn(planDate(File(d + u"same.jpg"_s)).kind == DatePlan::Kind::Same), u"y"_s);
    CHECK_EQ(yn(planDate(File(d + u"plain.txt"_s)).kind == DatePlan::Kind::NoDate), u"y"_s);
    CHECK_EQ(yn(planDate(File(d + u"zero.jpg"_s)).kind == DatePlan::Kind::NoDate), u"y"_s);
    CHECK_EQ(yn(planDate(File(d + u"sub"_s)).kind == DatePlan::Kind::Skipped), u"y"_s);
    CHECK_EQ(QString::number(mtime(d + u"p.jpg"_s)), QString::number(at(2025, 1, 1, 3, 0).toSecsSinceEpoch()));

    FileList fl;
    Journal j;
    fl.setJournal(&j);
    for (const char* n : {"p.jpg", "same.jpg", "plain.txt", "zero.jpg", "hid.jpg", "sub"}) {
        File& f = fl.add(d + QString::fromLatin1(n));
        f.hidden = QString::fromLatin1(n) == u"hid.jpg";
    }
    options().skipHidden = true;
    const qint64 size = QFileInfo(d + u"p.jpg"_s).size();
    CHECK_EQ(QString::number(fl.setDatesFromExif()), u"1"_s);
    CHECK_EQ(QString::number(mtime(d + u"p.jpg"_s)), QString::number(at(2023, 8, 12, 14, 20, 5).toSecsSinceEpoch()));
    CHECK_EQ(QString::number(QFileInfo(d + u"p.jpg"_s).size()), QString::number(size));  // the content is not touched
    CHECK_EQ(QString::number(mtime(d + u"plain.txt"_s)), QString::number(at(2024, 4, 4, 4, 4).toSecsSinceEpoch()));
    CHECK_EQ(QString::number(mtime(d + u"zero.jpg"_s)), QString::number(at(2024, 9, 9, 9, 9).toSecsSinceEpoch()));
    CHECK_EQ(QString::number(mtime(d + u"hid.jpg"_s)), QString::number(at(2024, 8, 8, 8, 8).toSecsSinceEpoch()));
    CHECK_EQ(QString::number(j.size()), u"1"_s);  // one pass, with the one file that changed
    CHECK_EQ(QString::number(j.passes().back().moves.size()), u"1"_s);
    CHECK_EQ(yn(j.passes().back().moves.front().date), u"y"_s);
    CHECK_EQ(fl.at(2).description().contains(u"no EXIF shooting date"_s) ? u"y"_s : u"n"_s, u"y"_s);

    // a second run changes nothing and leaves no pass
    CHECK_EQ(QString::number(fl.setDatesFromExif()), u"0"_s);
    CHECK_EQ(QString::number(j.size()), u"1"_s);

    // the journal on disk keeps the flag and the old date
    Journal j2;
    CHECK_EQ(yn(j2.fromJson(j.toJson())), u"y"_s);
    CHECK_EQ(yn(j2.passes().back().moves.front().date), u"y"_s);
    CHECK_EQ(QString::number(j2.passes().back().moves.front().oldMtime), QString::number(j.passes().back().moves.front().oldMtime));

    // the undo puts the old date back
    CHECK_EQ(QString::number(fl.undoPasses(1)), u"1"_s);
    CHECK_EQ(QString::number(mtime(d + u"p.jpg"_s)), QString::number(at(2025, 1, 1, 3, 0).toSecsSinceEpoch()));
    CHECK_EQ(QString::number(j.size()), u"0"_s);

    // a file edited after the change (its date moved again) is not touched by the undo
    CHECK_EQ(QString::number(fl.setDatesFromExif()), u"1"_s);
    QFile again(d + u"p.jpg"_s);
    CHECK_EQ(yn(again.open(QIODevice::ReadWrite)), u"y"_s);
    again.setFileTime(at(2026, 2, 2, 2, 2), QFileDevice::FileModificationTime);
    again.close();
    CHECK_EQ(QString::number(fl.undoPasses(1)), u"0"_s);
    CHECK_EQ(QString::number(mtime(d + u"p.jpg"_s)), QString::number(at(2026, 2, 2, 2, 2).toSecsSinceEpoch()));
    CHECK_EQ(QString::number(j.size()), u"1"_s);

    // a read-only file, and one that another program keeps open, still get their date (the content is never written)
    mkExif(d, u"ro.jpg"_s, u"2020:02:03 04:05:06"_s, u"000"_s);
    QFile rj(d + u"ro.jpg"_s);
    CHECK_EQ(yn(rj.open(QIODevice::ReadWrite)), u"y"_s);
    rj.setFileTime(at(2025, 3, 3, 3, 3), QFileDevice::FileModificationTime);
    rj.close();
    QFile::setPermissions(d + u"ro.jpg"_s, QFileDevice::ReadOwner);
    QFile held(d + u"ro.jpg"_s);
    CHECK_EQ(yn(held.open(QIODevice::ReadOnly)), u"y"_s);
    FileList fr;
    fr.add(d + u"ro.jpg"_s);
    CHECK_EQ(QString::number(fr.setDatesFromExif()), u"1"_s);
    held.close();
    CHECK_EQ(QString::number(mtime(d + u"ro.jpg"_s)), QString::number(at(2020, 2, 3, 4, 5, 6).toSecsSinceEpoch()));
    QFile::setPermissions(d + u"ro.jpg"_s, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    // the sidecars follow their picture, like for a renaming: a .xmp and a RAW get the date of the picture, in the list or not
    {
        QTemporaryDir ts;
        const QString e = ts.path() + u'/';
        mkExif(e, u"q.jpg"_s, u"2019:07:08 09:10:11"_s, u"000"_s);
        mk(e, u"q.xmp"_s, at(2025, 4, 4, 4, 4));
        mk(e, u"q.dng"_s, at(2025, 4, 4, 4, 4));
        mk(e, u"q.dng.xmp"_s, at(2025, 4, 4, 4, 4));  // the sidecar of the RAW, which is itself a sidecar of the picture
        mkExif(e, u"r.jpg"_s, u"2018:01:02 03:04:05"_s, u"000"_s);  // already right: only its sidecar changes
        mk(e, u"r.jpg.xmp"_s, at(2025, 5, 5, 5, 5));
        mk(e, u"s.dng"_s, at(2025, 6, 6, 6, 6));  // a RAW with its own sidecar, no EXIF: left alone
        mk(e, u"s.dng.xmp"_s, at(2025, 6, 6, 6, 6));
        for (const char* n : {"q.jpg", "r.jpg"}) {
            QFile jf(e + QString::fromLatin1(n));
            CHECK_EQ(yn(jf.open(QIODevice::ReadWrite)), u"y"_s);
            jf.setFileTime(QString::fromLatin1(n) == u"q.jpg" ? at(2025, 4, 4, 4, 4) : at(2018, 1, 2, 3, 4, 5),
                           QFileDevice::FileModificationTime);
        }
        options().renameSidecar = true;
        FileList fs;
        Journal js;
        fs.setJournal(&js);
        for (const char* n : {"q.jpg", "q.xmp", "q.dng", "r.jpg", "s.dng"}) {  // q.xmp and q.dng are in the list: q.jpg takes them
            fs.add(e + QString::fromLatin1(n));
        }
        const auto plans = planDates(fs);
        CHECK_EQ(yn(plans[0].kind == DatePlan::Kind::Change && plans[0].self), u"y"_s);
        CHECK_EQ(yn(plans[1].kind == DatePlan::Kind::Sidecar && plans[2].kind == DatePlan::Kind::Sidecar), u"y"_s);
        CHECK_EQ(yn(plans[3].kind == DatePlan::Kind::Change && !plans[3].self && plans[3].sidecars.size() == 1), u"y"_s);
        CHECK_EQ(QString::number(fs.setDatesFromExif()), u"2"_s);
        const qint64 q = at(2019, 7, 8, 9, 10, 11).toSecsSinceEpoch(), r = at(2018, 1, 2, 3, 4, 5).toSecsSinceEpoch();
        for (const char* n : {"q.jpg", "q.xmp", "q.dng", "q.dng.xmp"}) {
            CHECK_EQ(QString::number(mtime(e + QString::fromLatin1(n))), QString::number(q));
        }
        CHECK_EQ(QString::number(mtime(e + u"r.jpg.xmp"_s)), QString::number(r));
        CHECK_EQ(QString::number(mtime(e + u"s.dng.xmp"_s)), QString::number(at(2025, 6, 6, 6, 6).toSecsSinceEpoch()));
        CHECK_EQ(QString::number(js.size()), u"1"_s);
        Journal jt;  // the old dates of the sidecars survive the file on disk
        CHECK_EQ(yn(jt.fromJson(js.toJson())), u"y"_s);
        CHECK_EQ(QString::number(jt.passes().back().moves.front().sidecarDates.size()), u"3"_s);
        CHECK_EQ(QString::number(fs.setDatesFromExif()), u"0"_s);  // everything is right now
        CHECK_EQ(QString::number(fs.undoPasses(1)), u"1"_s);
        const qint64 old = at(2025, 4, 4, 4, 4).toSecsSinceEpoch();
        for (const char* n : {"q.jpg", "q.xmp", "q.dng", "q.dng.xmp"}) {
            CHECK_EQ(QString::number(mtime(e + QString::fromLatin1(n))), QString::number(old));
        }
        CHECK_EQ(QString::number(mtime(e + u"r.jpg.xmp"_s)), QString::number(at(2025, 5, 5, 5, 5).toSecsSinceEpoch()));
    }
}
