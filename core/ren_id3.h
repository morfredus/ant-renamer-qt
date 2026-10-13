#pragma once
#include <QString>

namespace ren {

struct Id3Info {
    QString title, author, album, year, comment, genreText;
    int track = 0;
    int genre = 0xFF;
};

QString id3Genre(int g);
bool readId3(const QString& path, Id3Info& out, QString* error = nullptr);

}  // namespace ren
