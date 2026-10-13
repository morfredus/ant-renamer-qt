#include "ren_exif.h"

#include <QFile>
#include <QHash>

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
    return false;
}

bool readExif(const QString& path, QMap<QString, QString>& tags) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    Reader r;
    r.d = f.read(512 * 1024);  // EXIF lives in the first APP1 segments
    if (r.d.size() < 12) return false;
    int tiff = -1;
    if (uchar(r.d[0]) == 0xFF && uchar(r.d[1]) == 0xD8) {  // JPEG: walk the segments
        int p = 2;
        while (p + 4 <= r.d.size() && uchar(r.d[p]) == 0xFF) {
            const int marker = uchar(r.d[p + 1]);
            const int len = uchar(r.d[p + 2]) << 8 | uchar(r.d[p + 3]);
            if (marker == 0xE1 && r.d.mid(p + 4, 6) == QByteArray("Exif\0\0", 6)) { tiff = p + 10; break; }
            if (marker == 0xDA || len < 2) break;
            p += 2 + len;
        }
    } else if (r.d.startsWith("II*") || r.d.startsWith("MM")) {
        tiff = 0;
    }
    if (tiff < 0 || tiff + 8 > r.d.size()) return false;
    r.base = tiff;
    r.le = r.d[tiff] == 'I';
    r.ifd(int(r.u32(tiff + 4)), tags);
    return !tags.isEmpty();
}

}  // namespace ren
