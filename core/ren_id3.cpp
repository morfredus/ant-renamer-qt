// ID3 reader (port of ExtractID3.pas, extended): ID3v2.2/2.3/2.4 at the start of the file, ID3v1/1.1 at the end.
// v2 values win; v1 fills what v2 does not have.
#include "ren_id3.h"

#include <QFile>

namespace ren {

static const char* const kGenres[127] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychadelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk/Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "Acapella",
    "Euro-House",
    "Dance Hall",
    "",
};

QString id3Genre(int g) {
    return (g >= 0 && g < 127) ? QString::fromLatin1(kGenres[g]) : QString();
}

namespace {

QString v1field(const QByteArray& b, int off, int len) {
    QByteArray s = b.mid(off, len);
    int z = s.indexOf('\0');
    if (z >= 0) s.truncate(z);
    return QString::fromLatin1(s);
}

bool readV1(QFile& f, Id3Info& out) {
    if (f.size() < 128) return false;
    f.seek(f.size() - 128);
    const QByteArray b = f.read(128);
    if (b.size() != 128 || !b.startsWith("TAG")) return false;
    out.title = v1field(b, 3, 30);
    out.author = v1field(b, 33, 30);
    out.album = v1field(b, 63, 30);
    out.year = v1field(b, 93, 4);  // blank years are NUL padded: they must not reach a file name
    out.comment = v1field(b, 97, 30);
    out.track = b[125] == '\0' ? static_cast<quint8>(b[126]) : 0;  // ID3v1.1 only: byte 125 is 0 and byte 126 the track (v1.0 has comment text there)
    out.genre = static_cast<quint8>(b[127]);
    out.genreText = id3Genre(out.genre);
    return true;
}

quint32 be32(const char* p) {
    return quint32(uchar(p[0])) << 24 | quint32(uchar(p[1])) << 16 | quint32(uchar(p[2])) << 8 | uchar(p[3]);
}

quint32 synchsafe(const char* p) {
    return quint32(uchar(p[0]) & 0x7F) << 21 | quint32(uchar(p[1]) & 0x7F) << 14 | quint32(uchar(p[2]) & 0x7F) << 7 | (uchar(p[3]) & 0x7F);
}

QByteArray unsync(QByteArray d) {  // FF 00 -> FF
    d.replace(QByteArray("\xFF\x00", 2), QByteArray("\xFF", 1));
    return d;
}

QString utf16(const QByteArray& s, bool bigEndian) {
    QString r;
    for (int i = 0; i + 1 < s.size(); i += 2)
        r += QChar(bigEndian ? ushort(uchar(s[i]) << 8 | uchar(s[i + 1])) : ushort(uchar(s[i + 1]) << 8 | uchar(s[i])));
    return r;
}

// Decodes one text field starting at `pos` for the given ID3 encoding byte and moves `pos` past its terminator.
QString decodeText(const QByteArray& d, int enc, int& pos) {
    const int n = int(d.size());
    int end = pos;
    if (enc == 1 || enc == 2) {  // UTF-16 with BOM / UTF-16BE
        while (end + 1 < n && !(d[end] == 0 && d[end + 1] == 0)) end += 2;
        QByteArray s = d.mid(pos, end - pos);
        pos = qMin(n, end + 2);
        if (enc == 2) return utf16(s, true);
        if (s.startsWith("\xFF\xFE")) return utf16(s.mid(2), false);
        if (s.startsWith("\xFE\xFF")) return utf16(s.mid(2), true);
        return utf16(s, false);
    }
    while (end < n && d[end] != 0) ++end;  // 0 = ISO-8859-1, 3 = UTF-8
    const QByteArray s = d.mid(pos, end - pos);
    pos = qMin(n, end + 1);
    return enc == 3 ? QString::fromUtf8(s) : QString::fromLatin1(s);
}

QString frameText(const QByteArray& d, bool comment) {
    if (d.isEmpty()) return {};
    const int enc = uchar(d[0]);
    int pos = 1;
    if (comment) {  // language (3 bytes) + short description, then the text
        pos += 3;
        decodeText(d, enc, pos);
    }
    if (pos >= d.size()) return {};
    return decodeText(d, enc, pos).trimmed();
}

QString v2Genre(const QString& t) {  // "(17)", "17", "(17)Rock", "Rock"
    const QString s = t.trimmed();
    if (s.startsWith(u'(')) {
        const int e = int(s.indexOf(u')'));
        if (e > 0) {
            bool ok = false;
            const int g = s.mid(1, e - 1).toInt(&ok);
            const QString rest = s.mid(e + 1).trimmed();
            if (!rest.isEmpty()) return rest;
            if (ok) return id3Genre(g);
        }
    }
    bool ok = false;
    const int g = s.toInt(&ok);
    return ok ? id3Genre(g) : s;
}

bool readV2(QFile& f, Id3Info& out) {
    f.seek(0);
    const QByteArray head = f.read(10);
    if (head.size() < 10 || !head.startsWith("ID3")) return false;
    const int ver = uchar(head[3]);
    const int flags = uchar(head[5]);
    if (ver < 2 || ver > 4) return false;
    const quint32 size = synchsafe(head.constData() + 6);
    QByteArray body = f.read(qMin<qint64>(size, 16 * 1024 * 1024));
    if (ver < 4 && (flags & 0x80)) body = unsync(body);  // whole-tag unsynchronisation (v2.2/2.3)
    int pos = 0;
    if (ver >= 3 && (flags & 0x40) && body.size() >= 4) {  // extended header: skip it
        // 64-bit arithmetic: a hostile size field must not wrap around to a negative offset (read before the buffer)
        const qint64 skip = ver == 4 ? qint64(synchsafe(body.constData())) : qint64(be32(body.constData())) + 4;
        pos = int(qMin<qint64>(skip, body.size()));
    }
    const int idLen = ver == 2 ? 3 : 4, hdr = ver == 2 ? 6 : 10;
    bool any = false;
    while (pos + hdr <= body.size() && body[pos] != 0) {
        const QByteArray id = body.mid(pos, idLen);
        const char* l = body.constData() + pos + idLen;
        quint32 len;
        if (ver == 2) len = uchar(l[0]) << 16 | uchar(l[1]) << 8 | uchar(l[2]);
        else if (ver == 3) len = be32(l);
        else len = synchsafe(l);
        const int fflags = ver == 2 ? 0 : uchar(body[pos + 9]);  // format flags
        const int dataStart = pos + hdr;
        if (len > quint32(body.size() - dataStart)) break;
        QByteArray data = body.mid(dataStart, int(len));
        pos = dataStart + int(len);
        if (ver == 4) {
            if (fflags & 0x0C) continue;       // compressed / encrypted: not supported
            if (fflags & 0x01) data.remove(0, 4);  // data length indicator
            if (fflags & 0x02) data = unsync(data);
        } else if (ver == 3 && (fflags & 0xC0)) {
            continue;                           // compressed / encrypted
        }

        auto is = [&](const char* a, const char* b) { return id == a || id == b; };
        if (is("TIT2", "TT2")) { out.title = frameText(data, false); any = true; }
        else if (is("TPE1", "TP1")) { out.author = frameText(data, false); any = true; }
        else if (is("TALB", "TAL")) { out.album = frameText(data, false); any = true; }
        else if (is("TYER", "TYE") || id == "TDRC") { out.year = frameText(data, false).left(4); any = true; }
        else if (is("TCON", "TCO")) { out.genreText = v2Genre(frameText(data, false)); out.genre = 0xFF; any = true; }
        else if (is("COMM", "COM")) { if (out.comment.isEmpty()) out.comment = frameText(data, true); any = true; }
        else if (is("TRCK", "TRK")) { out.track = frameText(data, false).section(u'/', 0, 0).toInt(); any = true; }
    }
    return any;
}

}  // namespace

bool readId3(const QString& path, Id3Info& out, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("unable to open file to read ID3 tag");
        return false;
    }
    Id3Info v1, v2;
    const bool has1 = readV1(f, v1);
    const bool has2 = readV2(f, v2);
    if (!has1 && !has2) {
        if (error) *error = QStringLiteral("no ID3 tag found in file");
        return false;
    }
    out = has1 ? v1 : Id3Info();
    if (has2) {
        auto take = [](QString& dst, const QString& src) { if (!src.isEmpty()) dst = src; };
        take(out.title, v2.title);
        take(out.author, v2.author);
        take(out.album, v2.album);
        take(out.year, v2.year);
        take(out.comment, v2.comment);
        take(out.genreText, v2.genreText);
        if (v2.track) out.track = v2.track;
    }
    return true;
}

}  // namespace ren
