// EPUB: a ZIP built here (one entry deflated through qCompress, one stored), read back, then the EPUB action.
// Included by test_core.cpp after CHECK_EQ is defined.
#pragma once

inline void epubChecks() {
    using namespace ren;
    auto le = [](quint32 v, int n) {
        QByteArray b;
        for (int i = 0; i < n; ++i) {
            b.append(char(v >> (8 * i)));
        }
        return b;
    };
    struct Entry {
        QString name;
        QByteArray data;
        bool deflate;
    };
    auto zip = [&](const QList<Entry>& entries) {
        QByteArray body, cd;
        for (const Entry& e : entries) {
            QByteArray comp = e.data;
            if (e.deflate) {
                comp = qCompress(e.data, 9).mid(6);  // 4-byte size + 2-byte zlib header, then raw deflate, then adler32
                comp.chop(4);
            }
            const QByteArray name = e.name.toUtf8();
            const quint32 method = e.deflate ? 8 : 0;
            const QByteArray common = le(method, 2) + le(0, 4) + le(0, 4) + le(comp.size(), 4) + le(e.data.size(), 4) + le(name.size(), 2);
            cd += QByteArray("PK\x01\x02", 4) + le(20, 2) + le(20, 2) + le(0, 2) + le(method, 2) + le(0, 4) + le(0, 4) +
                  le(comp.size(), 4) + le(e.data.size(), 4) + le(name.size(), 2) + le(0, 2) + le(0, 2) + le(0, 2) + le(0, 2) + le(0, 4) +
                  le(body.size(), 4) + name;
            body += QByteArray("PK\x03\x04", 4) + le(20, 2) + le(0, 2) + common.left(2) + le(0, 4) + le(0, 4) + le(comp.size(), 4) +
                    le(e.data.size(), 4) + le(name.size(), 2) + le(0, 2) + name + comp;
        }
        return body + cd + QByteArray("PK\x05\x06", 4) + le(0, 4) + le(entries.size(), 2) + le(entries.size(), 2) + le(cd.size(), 4) +
               le(body.size(), 4) + le(0, 2);
    };

    const QByteArray container =
        "<?xml version=\"1.0\"?><container xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\"><rootfiles>"
        "<rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/></rootfiles></container>";
    QByteArray opf =
        "<?xml version=\"1.0\"?><package xmlns=\"http://www.idpf.org/2007/opf\"><metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
        "xmlns:opf=\"http://www.idpf.org/2007/opf\">"
        "<dc:title>Le  Petit Prince</dc:title><dc:creator opf:role=\"aut\">Antoine de Saint-Exup\xC3\xA9ry</dc:creator>"
        "<dc:creator opf:role=\"ill\">Someone Else</dc:creator><dc:creator>Second Author</dc:creator>"
        "<dc:date>1943-04-06T00:00:00+00:00</dc:date><dc:publisher>Gallimard</dc:publisher><dc:language>fr</dc:language>"
        "<dc:identifier id=\"id\">urn:uuid:1234</dc:identifier><dc:identifier opf:scheme=\"ISBN\">9782070408504</dc:identifier>"
        "<meta name=\"calibre:series\" content=\"Contes\"/><meta name=\"calibre:series_index\" content=\"3.0\"/></metadata><manifest>";
    for (int i = 0; i < 300; ++i) {  // enough varied text for a real (dynamic Huffman) deflate block
        opf += "<item id=\"c" + QByteArray::number(i * 7919 % 1009) + "\" href=\"text/chapter" + QByteArray::number(i) +
               ".xhtml\" media-type=\"application/xhtml+xml\"/>";
    }
    opf += "</manifest></package>";

    QTemporaryDir td;
    auto write = [&](const QString& n, const QByteArray& d) {
        QFile f(td.filePath(n));
        if (!f.open(QIODevice::WriteOnly)) {
            CHECK_EQ(QStringLiteral("open failed"), QStringLiteral("ok"));
            return QString();
        }
        f.write(d);
        return f.fileName();
    };
    auto result = [](const EpubInfo& i) {
        return i.title + u'|' + i.author + u'|' + i.year + u'|' + i.publisher + u'|' + i.language + u'|' + i.isbn + u'|' + i.series + u'|' +
               i.seriesIndex;
    };
    const QString expected = QStringLiteral("Le Petit Prince|Antoine de Saint-Exupéry & Second Author|1943|Gallimard|fr|9782070408504|Contes|3.0");

    for (bool deflate : {true, false}) {
        EpubInfo info;
        const QString p = write(deflate ? u"d.epub"_s : u"s.epub"_s,
                                zip({{u"mimetype"_s, "application/epub+zip", false},
                                     {u"META-INF/container.xml"_s, container, deflate},
                                     {u"OEBPS/content.opf"_s, opf, deflate}}));
        CHECK_EQ(readEpub(p, info) ? result(info) : QStringLiteral("bad"), expected);
    }

    // the action: mask, series number padded
    {
        const QString p = write(u"book.epub"_s, zip({{u"META-INF/container.xml"_s, container, true}, {u"OEBPS/content.opf"_s, opf, true}}));
        ActionEpub a;
        a.mask = u"%author% - %series% %seriesnum% - %title% (%year%)%ext%"_s;
        a.twoDigit = true;
        CHECK_EQ(a.makeNewName(p, false), QStringLiteral("Antoine de Saint-Exupéry & Second Author - Contes 03 - Le Petit Prince (1943).epub"));
    }

    // no usable tag (blank title and author): the mask would give "- .epub", the file is left alone
    {
        const QByteArray blank = "<?xml version=\"1.0\"?><package xmlns=\"http://www.idpf.org/2007/opf\" version=\"2.0\"><metadata "
                                 "xmlns:dc=\"http://purl.org/dc/elements/1.1/\"><dc:title>   </dc:title><dc:creator></dc:creator>"
                                 "<dc:language>fr</dc:language></metadata></package>";
        const QString p = write(u"blank.epub"_s, zip({{u"META-INF/container.xml"_s, container, true}, {u"OEBPS/content.opf"_s, blank, true}}));
        ActionEpub a;
        a.mask = u"%author% - %title%%ext%"_s;
        CHECK_EQ(a.makeNewName(p, false), QString());
        a.mask = u"%author% - %title% (%lang%)%ext%"_s;  // a tag that is there: the name is made
        CHECK_EQ(a.makeNewName(p, false), QStringLiteral(" -  (fr).epub"));
    }

    // not an EPUB, truncated, hostile
    {
        EpubInfo info;
        QString err;
        CHECK_EQ(readEpub(write(u"x.epub"_s, "not a zip"), info, &err) ? u"ok"_s : err, QStringLiteral("not an EPUB file (no META-INF/container.xml)"));
        const QByteArray good = zip({{u"META-INF/container.xml"_s, container, true}, {u"OEBPS/content.opf"_s, opf, true}});
        CHECK_EQ(readEpub(write(u"t.epub"_s, good.left(good.size() / 2)), info) ? u"ok"_s : u"none"_s, QStringLiteral("none"));
        QByteArray junk = good;  // corrupt the deflate data: must fail cleanly, not crash
        for (int i = 40; i < junk.size() / 2; i += 7) {
            junk[i] = char(junk[i] ^ 0x5A);
        }
        readEpub(write(u"j.epub"_s, junk), info);  // reaching the next line is the check
        QByteArray out;
        CHECK_EQ(inflateRaw(QByteArray(100, '\xFF'), out, 1000) ? u"ok"_s : u"none"_s, QStringLiteral("none"));
    }
}
