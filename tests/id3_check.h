// ID3v2.3 (latin-1 + UTF-16 frames) with an ID3v1 fallback for what v2 lacks.
// Included by test_core.cpp after CHECK_EQ is defined.
#pragma once

inline void id3Checks() {
    using namespace ren;
    auto frame = [](const char* id, const QByteArray& data) {
        QByteArray f(id, 4);
        const int n = int(data.size());
        f.append(char(n >> 24)).append(char(n >> 16)).append(char(n >> 8)).append(char(n)).append(QByteArray(2, '\0'));
        return f + data;
    };
    const QByteArray latin1Title("\0Song", 5);
    const QByteArray utf16Artist("\x01\xFF\xFE" "A\0r\0t\0\xEF\0", 11);  // "Art" + i-diaeresis, little endian with BOM
    const QByteArray track("\0" "3/12", 5);
    const QByteArray genre("\0(17)", 5);
    const QByteArray body = frame("TIT2", latin1Title) + frame("TPE1", utf16Artist) + frame("TRCK", track) + frame("TCON", genre);
    const int n = int(body.size());
    QByteArray tag = QByteArray("ID3\x03\0\0", 6);
    tag.append(char((n >> 21) & 0x7F)).append(char((n >> 14) & 0x7F)).append(char((n >> 7) & 0x7F)).append(char(n & 0x7F));
    tag += body;
    QByteArray v1(128, '\0');
    v1.replace(0, 3, "TAG");
    v1.replace(63, 5, "Album");  // album only exists in v1

    QTemporaryDir td;
    QFile f(td.filePath(QStringLiteral("s.mp3")));
    if (!f.open(QIODevice::WriteOnly)) { CHECK_EQ(QStringLiteral("open failed"), QStringLiteral("ok")); return; }
    f.write(tag + QByteArray(100, 'x') + v1);
    f.close();
    Id3Info info;
    CHECK_EQ(readId3(f.fileName(), info) ? QStringLiteral("ok") : QStringLiteral("bad"), QStringLiteral("ok"));
    CHECK_EQ(info.title + u'|' + info.author + u'|' + info.album + u'|' + QString::number(info.track) + u'|' + info.genreText,
             QStringLiteral("Song|Artï|Album|3|Rock"));
}
