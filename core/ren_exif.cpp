#include "ren_exif.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QXmlStreamReader>

namespace ren {

namespace {

const QHash<int, QString>& tagNames() {
    static const QHash<int, QString> t = {
        {0x010E, "ImageDescription"}, {0x010F, "Make"}, {0x0110, "Model"}, {0x0112, "Orientation"},
        {0x011A, "XResolution"}, {0x011B, "YResolution"}, {0x0128, "ResolutionUnit"},
        {0x0131, "Software"}, {0x0132, "DateTime"}, {0x013B, "Artist"}, {0x8298, "Copyright"},
        {0x829A, "ExposureTime"}, {0x829D, "FNumber"}, {0x8822, "ExposureProgram"},
        {0x8827, "ISOSpeedRatings"}, {0x9000, "ExifVersion"}, {0x9003, "DateTimeOriginal"},
        {0x9004, "DateTimeDigitized"}, {0x9201, "ShutterSpeedValue"}, {0x9202, "ApertureValue"},
        {0x9204, "ExposureBiasValue"}, {0x9207, "MeteringMode"}, {0x9209, "Flash"},
        {0x920A, "FocalLength"}, {0x927C, "MakerNote"}, {0x9286, "UserComment"},
        {0xA001, "ColorSpace"}, {0xA002, "ExifImageWidth"}, {0xA003, "ExifImageLength"},
        {0xA402, "ExposureMode"}, {0xA403, "WhiteBalance"}, {0xA405, "FocalLengthIn35mmFilm"},
        {0xA406, "SceneCaptureType"}, {0xA431, "BodySerialNumber"}, {0xA433, "LensMake"},
        {0xA434, "LensModel"},
    };
    return t;
}

// IPTC-IIM (record 2) dataset number -> tag name.
const QHash<int, QString>& iptcNames() {
    static const QHash<int, QString> t = {
        {5, "IptcObjectName"}, {10, "IptcUrgency"}, {15, "IptcCategory"}, {20, "IptcSupplementalCategories"},
        {25, "IptcKeywords"}, {40, "IptcSpecialInstructions"}, {55, "IptcDateCreated"}, {60, "IptcTimeCreated"},
        {80, "IptcByline"}, {85, "IptcBylineTitle"}, {90, "IptcCity"}, {92, "IptcSublocation"}, {95, "IptcState"},
        {100, "IptcCountryCode"}, {101, "IptcCountry"}, {103, "IptcTransmissionReference"}, {105, "IptcHeadline"},
        {110, "IptcCredit"}, {115, "IptcSource"}, {116, "IptcCopyright"}, {120, "IptcCaption"}, {122, "IptcWriter"},
    };
    return t;
}

// XMP property ("namespace-uri local-name") -> tag name. isDate / isAlt tell how the value is read.
struct XmpProp { QString tag; bool isDate; bool isAlt; };
const QHash<QString, XmpProp>& xmpProps() {
    static const QString dc = "http://purl.org/dc/elements/1.1/ ", xmp = "http://ns.adobe.com/xap/1.0/ ",
        ps = "http://ns.adobe.com/photoshop/1.0/ ", core = "http://iptc.org/std/Iptc4xmpCore/1.0/xmlns/ ",
        lr = "http://ns.adobe.com/lightroom/1.0/ ", aux = "http://ns.adobe.com/exif/1.0/aux/ ";
    static const QHash<QString, XmpProp> t = {
        {dc + "title", {"XmpTitle", false, true}}, {dc + "description", {"XmpDescription", false, true}},
        {dc + "creator", {"XmpCreator", false, false}}, {dc + "rights", {"XmpRights", false, true}},
        {dc + "subject", {"XmpKeywords", false, false}},
        {xmp + "CreateDate", {"XmpCreateDate", true, false}}, {xmp + "ModifyDate", {"XmpModifyDate", true, false}},
        {xmp + "Rating", {"XmpRating", false, false}}, {xmp + "Label", {"XmpLabel", false, false}},
        {xmp + "CreatorTool", {"XmpCreatorTool", false, false}},
        {ps + "Headline", {"XmpHeadline", false, false}}, {ps + "Credit", {"XmpCredit", false, false}},
        {ps + "Source", {"XmpSource", false, false}}, {ps + "City", {"XmpCity", false, false}},
        {ps + "State", {"XmpState", false, false}}, {ps + "Country", {"XmpCountry", false, false}},
        {ps + "Category", {"XmpCategory", false, false}}, {ps + "DateCreated", {"XmpDateCreated", true, false}},
        {ps + "Instructions", {"XmpInstructions", false, false}},
        {ps + "AuthorsPosition", {"XmpAuthorsPosition", false, false}},
        {ps + "CaptionWriter", {"XmpCaptionWriter", false, false}},
        {ps + "TransmissionReference", {"XmpTransmissionReference", false, false}},
        {core + "Location", {"XmpLocation", false, false}}, {core + "CountryCode", {"XmpCountryCode", false, false}},
        {lr + "hierarchicalSubject", {"XmpHierarchicalKeywords", false, false}},
        {aux + "Lens", {"XmpLens", false, false}},
    };
    return t;
}

void addXmp(QMap<QString, QString>& out, const XmpProp& p, QString v) {
    v = v.trimmed();
    if (v.isEmpty() || out.contains(p.tag)) return;
    if (p.isDate) {  // ISO 8601 -> same layout as the EXIF dates
        const QDateTime dt = QDateTime::fromString(v, Qt::ISODate);
        if (dt.isValid()) v = dt.toString(v.size() <= 10 ? "yyyy:MM:dd" : "yyyy:MM:dd HH:mm:ss");
    }
    out.insert(p.tag, v);
}

void readXmp(const QByteArray& packet, QMap<QString, QString>& out) {
    static const QString rdf = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
    QXmlStreamReader r(packet);
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        if (r.namespaceUri() == rdf && r.name() == u"Description") {
            for (const auto& a : r.attributes()) {  // simple properties are written as attributes
                auto it = xmpProps().constFind(a.namespaceUri().toString() + u' ' + a.name().toString());
                if (it != xmpProps().constEnd()) addXmp(out, *it, a.value().toString());
            }
            continue;
        }
        auto it = xmpProps().constFind(r.namespaceUri().toString() + u' ' + r.name().toString());
        if (it == xmpProps().constEnd()) continue;
        QStringList items;  // rdf:li entries of an Alt/Bag/Seq, or the plain element text
        QString text;
        int depth = 1;
        while (!r.atEnd() && depth > 0) {
            r.readNext();
            if (r.isStartElement()) {
                if (r.namespaceUri() == rdf && r.name() == u"li") items << r.readElementText();
                else ++depth;
            } else if (r.isEndElement()) {
                --depth;
            } else if (r.isCharacters() && depth == 1) {
                text += r.text();
            }
        }
        if (items.isEmpty()) addXmp(out, *it, text);
        else addXmp(out, *it, it->isAlt ? items.first() : items.join(QStringLiteral(", ")));  // Alt: first = default language
    }
}

// IPTC-IIM block inside a Photoshop "8BIM" APP13 payload (`d` starts after "Photoshop 3.0\0").
void readIptc(const QByteArray& d, QMap<QString, QString>& out) {
    auto be16 = [&](int o) { return int(uchar(d[o])) << 8 | uchar(d[o + 1]); };
    int p = 0;
    while (p + 12 <= d.size() && d.mid(p, 4) == "8BIM") {
        const int id = be16(p + 4);
        const int nameLen = 1 + uchar(d[p + 6]);
        int q = p + 6 + nameLen + (nameLen % 2);  // pascal name is padded to an even length
        if (q + 4 > d.size()) return;
        const int size = int(uchar(d[q])) << 24 | uchar(d[q + 1]) << 16 | uchar(d[q + 2]) << 8 | uchar(d[q + 3]);
        q += 4;
        if (size < 0 || q + size > d.size()) return;
        if (id == 0x0404) {
            const QByteArray iim = d.mid(q, size);
            const bool utf8 = iim.contains(QByteArray("\x1c\x01\x5a\x00\x03\x1b%G", 8));  // 1:90 CodedCharacterSet = ESC % G
            QMap<QString, QStringList> found;
            for (int i = 0; i + 5 <= iim.size() && uchar(iim[i]) == 0x1C; ) {
                const int rec = uchar(iim[i + 1]), set = uchar(iim[i + 2]);
                const int len = int(uchar(iim[i + 3])) << 8 | uchar(iim[i + 4]);
                if ((len & 0x8000) || i + 5 + len > iim.size()) break;  // extended datasets: not used by the tags above
                const QByteArray v = iim.mid(i + 5, len);
                auto it = rec == 2 ? iptcNames().constFind(set) : iptcNames().constEnd();
                if (it != iptcNames().constEnd()) found[*it] << (utf8 ? QString::fromUtf8(v) : QString::fromLatin1(v)).trimmed();
                i += 5 + len;
            }
            for (auto it = found.begin(); it != found.end(); ++it) {
                QString v = it->join(QStringLiteral(", "));  // repeatable datasets (keywords, bylines...)
                if (it.key() == u"IptcDateCreated" && v.size() == 8) v = v.left(4) + u':' + v.mid(4, 2) + u':' + v.mid(6, 2);
                else if (it.key() == u"IptcTimeCreated" && v.size() >= 6) v = v.left(2) + u':' + v.mid(2, 2) + u':' + v.mid(4, 2);
                if (!v.isEmpty()) out.insert(it.key(), v);
            }
        }
        p = q + size + (size % 2);
    }
}

struct Reader {
    QByteArray d;
    int base = 0;       // offset of the TIFF header inside d
    bool le = true;

    quint16 u16(int o) const {
        if (o < 0 || o + 2 > d.size()) return 0;
        const uchar b0 = uchar(d.at(o)), b1 = uchar(d.at(o + 1));
        return le ? quint16(b0 | b1 << 8) : quint16(b1 | b0 << 8);
    }
    quint32 u32(int o) const {
        if (o < 0 || o + 4 > d.size()) return 0;
        return le ? u16(o) | quint32(u16(o + 2)) << 16 : quint32(u16(o)) << 16 | u16(o + 2);
    }

    QString value(int type, quint32 count, int entryOff) const {
        static const int sz[] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};
        if (type < 1 || type > 12 || count == 0 || count > 4096) return {};
        const quint64 bytes = quint64(sz[type]) * count;
        const int off = bytes <= 4 ? entryOff + 8 : base + int(u32(entryOff + 8));
        if (off < 0 || off + int(bytes) > d.size()) return {};
        switch (type) {
        case 2: {  // ASCII
            QByteArray s = d.mid(off, int(count));
            int z = s.indexOf('\0');
            if (z >= 0) s.truncate(z);
            return QString::fromLatin1(s).trimmed();
        }
        case 3: return QString::number(u16(off));
        case 4: return QString::number(u32(off));
        case 5: {
            quint32 n = u32(off), den = u32(off + 4);
            return den ? QString::number(double(n) / den, 'g', 6) : QString::number(n);
        }
        case 7: return count <= 4 ? QString::fromLatin1(d.mid(off, int(count))).trimmed() : QString();
        default: return {};
        }
    }

    void ifd(int off, QMap<QString, QString>& out, int depth = 0) const {
        if (depth > 2 || off <= 0) return;
        const int n = u16(base + off);
        for (int i = 0; i < n && i < 512; ++i) {
            const int e = base + off + 2 + i * 12;
            const int tag = u16(e), type = u16(e + 2);
            const quint32 count = u32(e + 4);
            if (tag == 0x8769) { ifd(int(u32(e + 8)), out, depth + 1); continue; }
            auto it = tagNames().constFind(tag);
            if (it == tagNames().constEnd()) continue;
            QString v = value(type, count, e);
            if (tag == 0x829A && type == 5) {  // ExposureTime reads better as 1/N
                const int o = base + int(u32(e + 8));
                quint32 nu = u32(o), de = u32(o + 4);
                if (nu && de > nu) v = QStringLiteral("1/%1").arg(qRound(double(de) / nu));
            }
            out.insert(*it, v);
        }
    }
};

}  // namespace

bool isKnownExifTag(const QString& name) {
    for (const QString& n : tagNames()) if (n.compare(name, Qt::CaseInsensitive) == 0) return true;
    for (const QString& n : iptcNames()) if (n.compare(name, Qt::CaseInsensitive) == 0) return true;
    for (const XmpProp& p : xmpProps()) if (p.tag.compare(name, Qt::CaseInsensitive) == 0) return true;
    return false;
}

const QStringList& sidecarExtensions() {
    static const QStringList e = {QStringLiteral("xmp"), QStringLiteral("dop")};
    return e;
}

QStringList sidecarCandidates(const QString& path, const QStringList& exts) {
    const QFileInfo fi(path);
    if (sidecarExtensions().contains(fi.suffix(), Qt::CaseInsensitive)) return {};
    const QString stem = fi.absolutePath() + u'/' + fi.completeBaseName();
    QStringList r;
    for (const QString& e : exts) r << stem + u'.' + e << stem + u'.' + e.toUpper() << path + u'.' + e << path + u'.' + e.toUpper();
    return r;
}

bool readExif(const QString& path, QMap<QString, QString>& tags) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    // A sidecar holds the edits of the photo manager (RAW files cannot carry them): it goes first, so it wins
    for (const QString& s : sidecarCandidates(path, {QStringLiteral("xmp")})) {
        QFile sf(s);
        if (!QFileInfo::exists(s) || !sf.open(QIODevice::ReadOnly)) continue;
        const QByteArray x = sf.read(1024 * 1024);
        const int xs = x.indexOf("<x:xmpmeta");
        if (xs >= 0) readXmp(x.mid(xs), tags);
    }
    Reader r;
    r.d = f.read(512 * 1024);  // metadata lives in the first segments
    if (r.d.size() < 12) return false;
    int tiff = -1;
    if (uchar(r.d[0]) == 0xFF && uchar(r.d[1]) == 0xD8) {  // JPEG: walk the segments
        int p = 2;
        while (p + 4 <= r.d.size() && uchar(r.d[p]) == 0xFF) {
            const int marker = uchar(r.d[p + 1]);
            const int len = uchar(r.d[p + 2]) << 8 | uchar(r.d[p + 3]);
            if (marker == 0xE1 && tiff < 0 && r.d.mid(p + 4, 6) == QByteArray("Exif\0\0", 6)) tiff = p + 10;
            if (marker == 0xED && r.d.mid(p + 4, 14) == QByteArray("Photoshop 3.0\0", 14))
                readIptc(r.d.mid(p + 18, len - 16), tags);
            if (marker == 0xDA || len < 2) break;
            p += 2 + len;
        }
    } else if (r.d.startsWith("II*") || r.d.startsWith("MM")) {
        tiff = 0;
    }
    if (tiff >= 0 && tiff + 8 <= r.d.size()) {
        r.base = tiff;
        r.le = r.d[tiff] == 'I';
        r.ifd(int(r.u32(tiff + 4)), tags);
    }
    // XMP is plain XML, found by content: this also covers PNG, WebP, TIFF, DNG and .xmp files
    const int xs = r.d.indexOf("<x:xmpmeta");
    const int xe = xs < 0 ? -1 : r.d.indexOf("</x:xmpmeta>", xs);
    if (xe > 0) readXmp(r.d.mid(xs, xe + 12 - xs), tags);
    return !tags.isEmpty();
}

}  // namespace ren
