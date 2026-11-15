#pragma once
// EPUB metadata reader: the EPUB is a ZIP, META-INF/container.xml points to the OPF file, the OPF holds Dublin Core
// fields. The ZIP reading and the inflate are done here, so the project stays free of any library but Qt.
#include <QByteArray>
#include <QString>

namespace ren {

struct EpubInfo {
    QString title, author, year, publisher, language, isbn, series, seriesIndex;
};

// Raw deflate (RFC 1951). false on corrupt data or when the result would exceed maxOut bytes.
bool inflateRaw(const QByteArray& in, QByteArray& out, qsizetype maxOut);

bool readEpub(const QString& path, EpubInfo& out, QString* error = nullptr);

}  // namespace ren
