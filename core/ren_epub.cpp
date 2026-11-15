#include "ren_epub.h"

#include <QFile>
#include <QRegularExpression>
#include <QUrl>
#include <QXmlStreamReader>

#include <cstring>

namespace ren {

// --- inflate (same method as zlib's "puff": canonical Huffman decoding, no tables) ---
namespace {

struct Bits {
    const uchar* in;
    qsizetype len, pos = 0;
    quint32 buf = 0;
    int cnt = 0;
    bool err = false;
    int get(int need) {
        quint32 v = buf;
        while (cnt < need) {
            if (pos >= len) {
                err = true;
                return 0;
            }
            v |= quint32(in[pos++]) << cnt;
            cnt += 8;
        }
        buf = v >> need;
        cnt -= need;
        return int(v & ((1u << need) - 1));
    }
};

struct Huff {
    short count[16];
    short symbol[288];
};

// 0 = complete code, > 0 = incomplete, < 0 = over-subscribed
int build(Huff& h, const short* length, int n) {
    std::memset(h.count, 0, sizeof h.count);
    for (int i = 0; i < n; ++i) {
        ++h.count[length[i]];
    }
    if (h.count[0] == n) {
        return 0;
    }
    int left = 1;
    for (int l = 1; l < 16; ++l) {
        left = (left << 1) - h.count[l];
        if (left < 0) {
            return left;
        }
    }
    short offs[16];
    offs[1] = 0;
    for (int l = 1; l < 15; ++l) {
        offs[l + 1] = short(offs[l] + h.count[l]);
    }
    for (int i = 0; i < n; ++i) {
        if (length[i]) {
            h.symbol[offs[length[i]]++] = short(i);
        }
    }
    return left;
}

int decode(Bits& b, const Huff& h) {
    int code = 0, first = 0, index = 0;
    for (int l = 1; l < 16; ++l) {
        code |= b.get(1);
        if (b.err) {
            return -1;
        }
        const int count = h.count[l];
        if (code - count < first) {
            return h.symbol[index + (code - first)];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

constexpr short kLBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                              31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr short kLExt[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr short kDBase[30] = {1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
                              193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
constexpr short kDExt[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

bool codes(Bits& b, QByteArray& out, qsizetype maxOut, const Huff& lit, const Huff& dist) {
    for (;;) {
        int s = decode(b, lit);
        if (s < 0) {
            return false;
        }
        if (s < 256) {
            if (out.size() >= maxOut) {
                return false;
            }
            out.append(char(s));
        } else if (s == 256) {
            return true;
        } else {
            s -= 257;
            if (s >= 29) {
                return false;
            }
            const int len = kLBase[s] + b.get(kLExt[s]);
            const int ds = decode(b, dist);
            if (ds < 0 || ds >= 30) {
                return false;
            }
            const qsizetype d = kDBase[ds] + b.get(kDExt[ds]);
            if (b.err || d > out.size() || out.size() + len > maxOut) {
                return false;
            }
            for (int i = 0; i < len; ++i) {
                out.append(out.at(out.size() - d));
            }
        }
    }
}

bool fixedBlock(Bits& b, QByteArray& out, qsizetype maxOut) {
    static Huff lit, dist;
    static const bool built = [] {
        short l[288], d[30];
        int i = 0;
        for (; i < 144; ++i) l[i] = 8;
        for (; i < 256; ++i) l[i] = 9;
        for (; i < 280; ++i) l[i] = 7;
        for (; i < 288; ++i) l[i] = 8;
        build(lit, l, 288);
        for (i = 0; i < 30; ++i) d[i] = 5;
        build(dist, d, 30);
        return true;
    }();
    (void)built;
    return codes(b, out, maxOut, lit, dist);
}

bool dynamicBlock(Bits& b, QByteArray& out, qsizetype maxOut) {
    static const short order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    const int nlen = b.get(5) + 257, ndist = b.get(5) + 1, ncode = b.get(4) + 4;
    if (b.err || nlen > 286 || ndist > 30) {
        return false;
    }
    short lengths[320] = {};
    for (int i = 0; i < ncode; ++i) {
        lengths[order[i]] = short(b.get(3));
    }
    Huff lencode;
    if (b.err || build(lencode, lengths, 19) != 0) {
        return false;
    }
    int idx = 0;
    while (idx < nlen + ndist) {
        const int sym = decode(b, lencode);
        if (sym < 0) {
            return false;
        }
        if (sym < 16) {
            lengths[idx++] = short(sym);
            continue;
        }
        int rep, val = 0;
        if (sym == 16) {
            if (idx == 0) {
                return false;
            }
            val = lengths[idx - 1];
            rep = 3 + b.get(2);
        } else if (sym == 17) {
            rep = 3 + b.get(3);
        } else {
            rep = 11 + b.get(7);
        }
        if (b.err || idx + rep > nlen + ndist) {
            return false;
        }
        while (rep--) {
            lengths[idx++] = short(val);
        }
    }
    if (lengths[256] == 0) {
        return false;
    }
    Huff lit, dist;
    const int e1 = build(lit, lengths, nlen);
    if (e1 < 0 || (e1 > 0 && nlen - lit.count[0] != 1)) {
        return false;
    }
    const int e2 = build(dist, lengths + nlen, ndist);
    if (e2 < 0 || (e2 > 0 && ndist - dist.count[0] != 1)) {
        return false;
    }
    return codes(b, out, maxOut, lit, dist);
}

}  // namespace

bool inflateRaw(const QByteArray& in, QByteArray& out, qsizetype maxOut) {
    Bits b{reinterpret_cast<const uchar*>(in.constData()), in.size()};
    out.clear();
    int last;
    do {
        last = b.get(1);
        const int type = b.get(2);
        if (b.err) {
            return false;
        }
        bool ok;
        if (type == 0) {
            b.buf = 0;
            b.cnt = 0;
            if (b.pos + 4 > b.len) {
                return false;
            }
            const int n = b.in[b.pos] | (b.in[b.pos + 1] << 8);
            const int nn = b.in[b.pos + 2] | (b.in[b.pos + 3] << 8);
            b.pos += 4;
            ok = n == (~nn & 0xFFFF) && b.pos + n <= b.len && out.size() + n <= maxOut;
            if (ok) {
                out.append(reinterpret_cast<const char*>(b.in) + b.pos, n);
                b.pos += n;
            }
        } else if (type == 1) {
            ok = fixedBlock(b, out, maxOut);
        } else if (type == 2) {
            ok = dynamicBlock(b, out, maxOut);
        } else {
            ok = false;
        }
        if (!ok) {
            return false;
        }
    } while (!last);
    return true;
}

// --- ZIP: one entry, by its name, through the central directory ---
namespace {

quint32 u16(const QByteArray& d, qsizetype p) {
    return p + 2 <= d.size() ? quint32(uchar(d[p])) | (quint32(uchar(d[p + 1])) << 8) : 0;
}
quint32 u32(const QByteArray& d, qsizetype p) {
    return u16(d, p) | (u16(d, p + 2) << 16);
}

constexpr qsizetype kMaxEntry = 4 * 1024 * 1024;  // container.xml and OPF files are small: a bigger one is not what we want

bool zipEntry(QFile& f, const QString& name, QByteArray& out) {
    const qint64 size = f.size();
    const qint64 tailLen = qMin<qint64>(size, 22 + 65535);
    if (!f.seek(size - tailLen)) {
        return false;
    }
    const QByteArray tail = f.read(tailLen);
    const qsizetype eocd = tail.lastIndexOf(QByteArray("PK\x05\x06", 4));
    if (eocd < 0) {
        return false;
    }
    const quint32 count = u16(tail, eocd + 10), cdSize = u32(tail, eocd + 12), cdOff = u32(tail, eocd + 16);
    if (qint64(cdOff) + cdSize > size || cdSize > 16 * 1024 * 1024 || !f.seek(cdOff)) {
        return false;
    }
    const QByteArray cd = f.read(cdSize);
    qsizetype p = 0;
    for (quint32 i = 0; i < count && p + 46 <= cd.size(); ++i) {
        if (u32(cd, p) != 0x02014b50) {
            return false;
        }
        const quint32 method = u16(cd, p + 10), comp = u32(cd, p + 20), raw = u32(cd, p + 24), local = u32(cd, p + 42);
        const qsizetype nameLen = u16(cd, p + 28), skip = nameLen + u16(cd, p + 30) + u16(cd, p + 32);
        const QString entry = QString::fromUtf8(cd.mid(p + 46, nameLen));
        p += 46 + skip;
        if (entry != name) {
            continue;
        }
        if (raw > kMaxEntry || comp > size || !f.seek(local)) {  // (a zip64 entry has 0xFFFFFFFF here: refused)
            return false;
        }
        const QByteArray lh = f.read(30);
        if (u32(lh, 0) != 0x04034b50 || !f.seek(qint64(local) + 30 + u16(lh, 26) + u16(lh, 28))) {
            return false;
        }
        const QByteArray data = f.read(comp);
        if (data.size() != qsizetype(comp)) {
            return false;
        }
        if (method == 0) {
            out = data;
            return true;
        }
        return method == 8 && inflateRaw(data, out, kMaxEntry);
    }
    return false;
}

QString attr(const QXmlStreamReader& x, const char* local) {
    for (const auto& a : x.attributes()) {
        if (a.name() == QLatin1String(local)) {
            return a.value().toString();
        }
    }
    return {};
}

}  // namespace

bool readEpub(const QString& path, EpubInfo& out, QString* error) {
    auto fail = [error](const char* msg) {
        if (error) {
            *error = QString::fromLatin1(msg);
        }
        return false;
    };
    out = EpubInfo();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return fail("unable to open file to read EPUB metadata");
    }
    QByteArray xml;
    if (!zipEntry(f, QStringLiteral("META-INF/container.xml"), xml)) {
        return fail("not an EPUB file (no META-INF/container.xml)");
    }
    QString opfPath;
    {
        QXmlStreamReader x(xml);
        while (!x.atEnd() && opfPath.isEmpty()) {
            if (x.readNext() == QXmlStreamReader::StartElement && x.name() == QLatin1String("rootfile")) {
                opfPath = QUrl::fromPercentEncoding(attr(x, "full-path").toUtf8());
            }
        }
    }
    if (opfPath.isEmpty() || !zipEntry(f, opfPath, xml)) {
        return fail("no OPF metadata found in EPUB file");
    }

    QStringList authors;
    QXmlStreamReader x(xml);
    while (!x.atEnd()) {
        if (x.readNext() != QXmlStreamReader::StartElement) {
            continue;
        }
        const QString el = x.name().toString();
        if (el == u"meta") {  // Calibre (name/content) or EPUB 3 (property + text)
            const QString name = attr(x, "name"), prop = attr(x, "property"), content = attr(x, "content");
            if (name == u"calibre:series" && out.series.isEmpty()) {
                out.series = content.trimmed();
            } else if (name == u"calibre:series_index" && out.seriesIndex.isEmpty()) {
                out.seriesIndex = content.trimmed();
            } else if (prop == u"belongs-to-collection" && out.series.isEmpty()) {
                out.series = x.readElementText().trimmed();
            } else if (prop == u"group-position" && out.seriesIndex.isEmpty()) {
                out.seriesIndex = x.readElementText().trimmed();
            }
            continue;
        }
        if (el != u"title" && el != u"creator" && el != u"date" && el != u"publisher" && el != u"language" && el != u"identifier") {
            continue;
        }
        const QString role = attr(x, "role"), scheme = attr(x, "scheme");
        const QString v = x.readElementText().simplified();
        if (v.isEmpty()) {
            continue;
        }
        if (el == u"title" && out.title.isEmpty()) {
            out.title = v;
        } else if (el == u"creator" && (role.isEmpty() || role == u"aut")) {
            authors << v;
        } else if (el == u"date" && out.year.isEmpty()) {
            static const QRegularExpression year(QStringLiteral("\\d{4}"));
            out.year = year.match(v).captured(0);
        } else if (el == u"publisher" && out.publisher.isEmpty()) {
            out.publisher = v;
        } else if (el == u"language" && out.language.isEmpty()) {
            out.language = v;
        } else if (el == u"identifier" && out.isbn.isEmpty() && (scheme.compare(u"ISBN", Qt::CaseInsensitive) == 0 || v.startsWith(u"urn:isbn:", Qt::CaseInsensitive))) {
            out.isbn = v.startsWith(u"urn:isbn:", Qt::CaseInsensitive) ? v.mid(9) : v;
        }
    }
    out.author = authors.join(QStringLiteral(" & "));
    return true;
}

}  // namespace ren
